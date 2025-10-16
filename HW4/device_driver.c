#include <linux/cdev.h>           // 문자 디바이스 등록/제거
#include <linux/delay.h>          // msleep 함수 포함
#include <linux/device.h>         // sysfs 연동 및 /dev 노드 생성
#include <linux/fcntl.h>          // 파일 열기 관련 플래그 정의
#include <linux/fs.h>             // file_operations 구조체 및 파일 연산 정의
#include <linux/gpio/consumer.h>  // gpio 제어를 위한 gpiod 함수
#include <linux/init.h>           // __init, __exit 등의 매크로 정의
#include <linux/module.h>
#include <linux/uaccess.h>  // 데이터 복사 함수 포함

#define DRIVER_NAME "gpiomorse"       // 문자 디바이스 이름
#define CLASS_NAME "gpiomorse_class"  // sysfs에 등록될 클래스 이름
#define DEVICE_NAME "gpiomorse"       // /dev/ 아래 생성될 디바이스 이름
#define TX_BCM 17                     // 출력, 송신용
#define RX_BCM 26                     // 입력, 수신용
#define GPIOCHIP_BASE 512             // BCM 번호를 내부 GPIO 번호로 바꿀 때 쓸 베이스 값
#define BIT_DELAY_MS 200              // 비트당 신호 지속 시간(ms)

static dev_t dev_num;                      // 문자 디바이스 번호
static struct cdev gpio_cdev;              // 문자 디바이스 구조체
static struct class *gpio_class;           // 디바이스 클래스
static struct device *gpio_device;         // 디바이스 객체
static struct fasync_struct *async_queue;  // SIGIO를 위한 큐
struct gpio_desc *gpio_tx;                 // 송신 GPIO 디스크립터
struct gpio_desc *gpio_rx;                 // 수신 GPIO 디스크립터

static const char *morse_table[52] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-", "-.--", "--..",
};

// 문자 -> 6비트 숫자 매핑 (A-Z:0~25, a-z:26~51)
static int char_to_sixbit(char c) {
    if (c >= 'a' && c <= 'z')  // 소문자면 대문자로
        c -= 32;
    if (c >= 'A' && c <= 'Z') return c - 'A';
    return -1;
}

// 6비트 숫자 -> 문자 복원
static char sixbit_to_char(int val) {
    if (val >= 0 && val < 26) return 'A' + val;  // 일관되게 대문자 출력
    return '?';
}

// 6비트 숫자를 LSB부터 TX 핀으로 전송
static void send_sixbit_serial(int sixbit) {
    int i;
    for (i = 0; i < 6; i++) {
        int bit = (sixbit >> i) & 1;
        gpiod_set_value(gpio_tx, bit);
        msleep(BIT_DELAY_MS);
    }
    gpiod_set_value(gpio_tx, 0);
    msleep(BIT_DELAY_MS);
}

// RX 핀에서 6비트 시리얼 비트 읽어 숫자 복원
static int receive_sixbit_serial(void) {
    int val, sixbit = 0;
    int i;

    for (i = 0; i < 6; i++) {
        msleep(BIT_DELAY_MS);
        val = gpiod_get_value(gpio_rx);
        sixbit |= (val & 0x1) << i;
    }
    return sixbit;
}

// 6비트 숫자를 모스부호 문자열로 변환
static void sixbit_to_morse(int sixbit, char *buf, size_t buf_size) {
    if (sixbit < 0 || sixbit >= 52) {
        snprintf(buf, buf_size, "?");
        return;
    }
    snprintf(buf, buf_size, "%s", morse_table[sixbit]);
}

static int gpio_open(struct inode *inode, struct file *filp) {
    pr_info("[gpiomorse] Device opened\n");
    return 0;
}

static int gpio_close(struct inode *inode, struct file *filp) {
    fasync_helper(-1, filp, 0, &async_queue);
    pr_info("[gpiomorse] Device closed\n");
    return 0;
}

static int gpio_fasync(int fd, struct file *filp, int mode) { return fasync_helper(fd, filp, mode, &async_queue); }

// write: 유저 공간에서 받은 문자열 -> 6비트 숫자 인코딩 -> 시리얼 비트 송신
static ssize_t gpio_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    char kbuf[64];
    int sixbit;
    int i;

    if (!gpio_tx) return -ENODEV;

    if (len > sizeof(kbuf) - 1) return -EINVAL;

    if (copy_from_user(kbuf, buf, len)) return -EFAULT;

    kbuf[len] = '\0';

    if (gpiod_get_direction(gpio_tx) != 0) return -EPERM;

    for (i = 0; i < len; i++) {
        sixbit = char_to_sixbit(kbuf[i]);
        if (sixbit == -1) {
            pr_warn("[gpiomorse] Ignoring non-alphabet character: '%c'\n", kbuf[i]);
            continue;
        }
        send_sixbit_serial(sixbit);
    }
    return len;
}

// read: RX 핀에서 6비트 숫자 수신 -> 모스부호 문자열 변환 -> 유저 공간에 전달
static ssize_t gpio_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    int sixbit;
    char morse[16];
    size_t morse_len;

    if (!gpio_rx) return -ENODEV;

    sixbit = receive_sixbit_serial();

    sixbit_to_morse(sixbit, morse, sizeof(morse));

    morse_len = strlen(morse);
    if (morse_len > len) return -EINVAL;

    if (copy_to_user(buf, morse, morse_len)) return -EFAULT;
    if (async_queue) kill_fasync(&async_queue, SIGIO, POLL_IN);
    return morse_len;
}

static const struct file_operations gpio_fops = {
    .owner = THIS_MODULE,
    .open = gpio_open,
    .release = gpio_close,
    .read = gpio_read,
    .write = gpio_write,
    .fasync = gpio_fasync,
};

static int __init gpio_driver_init(void) {  // insmod
    int ret;
    int tx_gpio_num = GPIOCHIP_BASE + TX_BCM;
    int rx_gpio_num = GPIOCHIP_BASE + RX_BCM;

    pr_info("[gpiomorse] Module loading\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, DRIVER_NAME);
    if (ret) return ret;

    cdev_init(&gpio_cdev, &gpio_fops);
    gpio_cdev.owner = THIS_MODULE;

    ret = cdev_add(&gpio_cdev, dev_num, 1);
    if (ret) goto unregister_chrdev;

    gpio_class = class_create(CLASS_NAME);
    if (IS_ERR(gpio_class)) {
        ret = PTR_ERR(gpio_class);
        goto del_cdev;
    }

    gpio_device = device_create(gpio_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(gpio_device)) {
        ret = PTR_ERR(gpio_device);
        goto destroy_class;
    }

    gpio_tx = gpio_to_desc(tx_gpio_num);
    gpio_rx = gpio_to_desc(rx_gpio_num);

    if (!gpio_tx || !gpio_rx) {
        pr_err("[gpiomorse] Failed to get GPIO descriptors\n");
        ret = -ENODEV;
        goto destroy_device;
    }

    gpiod_direction_output(gpio_tx, 0);
    gpiod_direction_input(gpio_rx);

    pr_info("[gpiomorse] GPIO morse driver initialized\n");
    return 0;

destroy_device:
    device_destroy(gpio_class, dev_num);
destroy_class:
    class_destroy(gpio_class);
del_cdev:
    cdev_del(&gpio_cdev);
unregister_chrdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void __exit gpio_driver_exit(void) {  // rmmod
    device_destroy(gpio_class, dev_num);
    class_destroy(gpio_class);
    cdev_del(&gpio_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("[gpiomorse] Module unloaded\n");
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Bak MinJeong");
MODULE_DESCRIPTION("SystemProgramming assignment 4");
