/*버튼을 눌렀을 때 LED의 Toggle
Export로 17,20,21핀을 활성화한다.
Direction으로 버튼을 입력 핀으로 사용하고, LED를 아웃핀으로 사용한다.
버튼의 사용을 위해 21핀은 늘 HIGH상태로 설정해야한다.
*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define PIN 20    // Btn
#define POUT 17   // LED 연결, 액츄에이터로 사용
#define POUT2 21  // Btn 전압 주는 핀 (GPIO21)
#define VALUE_MAX 40
#define DIRECTION_MAX 128
#define BUFFER_MAX 3

static int GPIOExport(int pin);
static int GPIOUnexport(int pin);
static int GPIODirection(int pin, int dir);
static int GPIOWrite(int pin, int value);
static int GPIORead(int pin);

int main(int argc, char* argv[]) {
    int state = 1, prev_state = 1;  //
    int light = 0;
    int presstime = 0;

    if (GPIOExport(PIN) || GPIOExport(POUT) == -1 || GPIOExport(POUT2) == -1) {  // GPIO export 설정
        return 1;
    }
    if (GPIODirection(PIN, IN) == -1 || GPIODirection(POUT, OUT) == -1 || GPIODirection(POUT2, OUT) == -1) {
        return 2;  // GPIO Direction 설정
    }
    GPIOWrite(POUT, LOW);  // 초기 프로그램 시작시, LED OFF
    GPIOWrite(POUT2, HIGH);

    while (1) {
        state = GPIORead(PIN);  // 버튼값 읽음
        if (GPIOWrite(POUT2, 1) == -1) {
            return 3;
        }
        if (state == 0) {           // 1.현재 버튼이 눌려있으면
            if (prev_state == 1) {  // 1-a.prev_state==1이면
                light = (!light);   // 토글
                GPIOWrite(POUT, light);
            } else {
                presstime += 100;                 // 1-b.prev_state==0이면 : 계속 누르고 있을시
                if (presstime >= 800) {           // 800ms 이상 눌렀을시
                    while (GPIORead(PIN) == 0) {  // 버튼이 눌려있는 동안
                        light = !light;
                        GPIOWrite(POUT, light);  // LED 점멸
                        usleep(500 * 1000);      // 500ms 간격으로
                    }
                }
            }
        } else {  // 2.버튼이 안 눌려있으면 시간 초기화 및 아무변화 x.
            presstime = 0;
        }
        usleep(100 * 1000);
        prev_state = state;
    }

    if (GPIOUnexport(PIN) == -1 || GPIOUnexport(POUT) == -1 || GPIOUnexport(POUT2) == -1) {
        return 4;
    }

    return 0;
}

static int GPIOExport(int pin) {  // 커널에 GPIO의 제어권 요청
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/sysprog_gpio/export", O_WRONLY);  // gpio제어용 export 파일을 wronly로 연다.
    if (-1 == fd) {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);  // gpio핀 번호를 문자열로 바꿔 buffer에 저장
    //%d는 정수를 문자열로 변환하는 포맷 지정자
    write(fd, buffer, bytes_written);  // 그 문자열을 해당 폴더에 씀 > export 완료함.
    close(fd);

    return (0);
}
static int GPIOUnexport(int pin) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/sysprog_gpio/unexport", O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open unexport for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);

    return (0);
}

static int GPIODirection(int pin, int dir) {  // Data direction설정. 센서와 액추에이터 결정
    static const char s_directions_str[] = "in\0out";
    // 문자열 배열 생성 : i, n \0, o, u, t, \0
    char path[DIRECTION_MAX];
    int fd;
    // snprintf는 Formatted strin을 버퍼에 저장하는 함수
    snprintf(path, DIRECTION_MAX, "/dev/gpio%d", pin);  // 경로 생성
    fd = open(path, O_WRONLY);                          // 생성한 경로로 fd생성,
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio direction for writing! %d\n", pin);
        return (-1);
    }
    if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
        // fd가 in이면 0, Out이면 3, in이면 길이 2, Out이면 길이 3
        fprintf(stderr, "Failed to set direction!\n");
        return (-1);
    }

    close(fd);

    return (0);
}

static int GPIOWrite(int pin, int value) {  // gpio에 특정값을 write
    static const char s_values_str[] = "01";
    char path[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/dev/gpio%d", pin);
    fd = open(path, O_WRONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    }
    if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
        // low면 0, high면 1 write.
        fprintf(stderr, "Failed to write value!\n");
        return (-1);
    }

    close(fd);

    return (0);
}

static int GPIORead(int pin) {
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/dev/gpio%d", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return (-1);
    }
    if (-1 == read(fd, value_str, 3)) {
        // 3바이트 상태를 읽어옴. 1\n, 0\n
        fprintf(stderr, "Failed to read value!\n");
        return (-1);
    }

    close(fd);

    return (atoi(value_str));
}