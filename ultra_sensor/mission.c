// 초음파센서 & LED 제어 :측정 거리가 작을수록 LED가 밝아지고 클수록 어두워지는 예제
//  스레드1: HC-SR04 거리 측정 -> distance 갱신
//  스레드2: distance 기반 LED PWM

#include <gpiod.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// 핀설정
#define PIN_TRIG 23
#define PIN_ECHO 24
#define PIN_LED 18

#define DIST_MIN 5.0            // 이보다 가까우면 최대 밝기
#define DIST_MAX 200.0          // 이보다 멀면 최소 밝기
#define PWM_PERIOD_NS 2000000L  // pwm 1주기

static double distance = 999.0;
static pthread_mutex_t dist_mx = PTHREAD_MUTEX_INITIALIZER;  // mutex 구현
static volatile sig_atomic_t g_stop = 0;                     // mutex구현

// 핀 포인터 선언
static struct gpiod_chip* chip = NULL;
static struct gpiod_line* line_trig = NULL;
static struct gpiod_line* line_echo = NULL;
static struct gpiod_line* line_led = NULL;

static void nsleep(long ns) {  // 나노초로 변환.
    struct timespec ts;
    ts.tv_sec = ns / 1000000000L;
    ts.tv_nsec = ns % 1000000000L;
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
    // 현재 시점부터 ts 만큼 스레드를 sleep -> pwm 유지
}

static void on_sigint(int signo) {  // 시그널 핸들러, 안전종료
    (void)signo;
    g_stop = 1;
}

// 시간차로 거리계산
static inline long diff_timespec_ns(const struct timespec* a, const struct timespec* b) { return (a->tv_sec - b->tv_sec) * 1000000000L + (a->tv_nsec - b->tv_nsec); }

// 스레드1: 초음파 거리 측정
static void* th_measure(void* arg) {
    (void)arg;

    // 트리거는 출력, 에코는 이벤트 입력
    if (gpiod_line_request_output(line_trig, "ultraled_trig", 0) < 0) {
        perror("gpiod_line_request_output(TRIG)");
        g_stop = 1;
        return NULL;
    }
    if (gpiod_line_request_both_edges_events(line_echo, "ultraled_echo") < 0) {
        perror("gpiod_line_request_both_edges_events(ECHO)");
        g_stop = 1;
        return NULL;
    }

    while (!g_stop) {
        // 1) TRIG low로 안정화
        gpiod_line_set_value(line_trig, 0);
        nsleep(2000 * 1000L);  // 2ms

        // 2) 10us HIGH 펄스
        gpiod_line_set_value(line_trig, 1);
        nsleep(10 * 1000L);  // 10us
        gpiod_line_set_value(line_trig, 0);

        // 3) Rising edge 대기
        struct timespec timeout;
        timeout.tv_sec = 0;
        timeout.tv_nsec = 60 * 1000 * 1000L;  // 60ms
        int ret = gpiod_line_event_wait(line_echo, &timeout);
        if (ret <= 0) {
            pthread_mutex_lock(&dist_mx);
            distance = DIST_MAX + 1.0;
            pthread_mutex_unlock(&dist_mx);
            continue;
        }

        struct gpiod_line_event ev_rise;
        if (gpiod_line_event_read(line_echo, &ev_rise) < 0 || ev_rise.event_type != GPIOD_LINE_EVENT_RISING_EDGE) continue;

        // 4) Falling edge 대기
        ret = gpiod_line_event_wait(line_echo, &timeout);
        if (ret <= 0) {
            pthread_mutex_lock(&dist_mx);
            distance = DIST_MAX + 1.0;
            pthread_mutex_unlock(&dist_mx);
            continue;
        }

        struct gpiod_line_event ev_fall;
        if (gpiod_line_event_read(line_echo, &ev_fall) < 0 || ev_fall.event_type != GPIOD_LINE_EVENT_FALLING_EDGE) continue;

        // 5) 펄스폭(ns) → 거리(cm)
        long dt_ns = diff_timespec_ns(&ev_fall.ts, &ev_rise.ts);
        double pulse_us = (double)dt_ns / 1000.0;
        double dist = (pulse_us * 0.0343) / 2.0;  // cm

        if (dist < 0.0) dist = DIST_MAX + 1.0;

        pthread_mutex_lock(&dist_mx);
        distance = dist;
        pthread_mutex_unlock(&dist_mx);

        nsleep(50 * 1000 * 1000L);  // 50ms
    }

    gpiod_line_release(line_trig);
    gpiod_line_release(line_echo);
    return NULL;
}

// 거리 → 듀티(0.0~1.0) 매핑
static double distance_to_duty(double d) {
    if (d <= DIST_MIN) return 1.0;
    if (d >= DIST_MAX) return 0.0;
    double t = (d - DIST_MIN) / (DIST_MAX - DIST_MIN);
    return 1.0 - t;
}

// 스레드2: LED 소프트 PWM (libgpiod 기반)
static void* th_pwm(void* arg) {
    (void)arg;

    if (gpiod_line_request_output(line_led, "ultraled_led", 0) < 0) {
        perror("gpiod_line_request_output(LED)");
        g_stop = 1;
        return NULL;
    }

    const long period_ns = PWM_PERIOD_NS;
    struct timespec wake;
    clock_gettime(CLOCK_MONOTONIC, &wake);

    while (!g_stop) {
        double d;
        pthread_mutex_lock(&dist_mx);
        d = distance;
        pthread_mutex_unlock(&dist_mx);

        double duty = distance_to_duty(d);
        if (duty < 0.0) duty = 0.0;
        if (duty > 1.0) duty = 1.0;

        long duty_ns = (long)(period_ns * duty);
        if (duty_ns >= period_ns) duty_ns = period_ns - 1;
        long off_ns = period_ns - duty_ns;

        // HIGH 구간
        if (duty_ns > 0) {
            gpiod_line_set_value(line_led, 1);
            wake.tv_nsec += duty_ns;
            while (wake.tv_nsec >= 1000000000L) {
                wake.tv_nsec -= 1000000000L;
                wake.tv_sec++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, NULL);
        } else {
            gpiod_line_set_value(line_led, 0);
        }

        // LOW 구간
        if (off_ns > 0) {
            gpiod_line_set_value(line_led, 0);
            wake.tv_nsec += off_ns;
            while (wake.tv_nsec >= 1000000000L) {
                wake.tv_nsec -= 1000000000L;
                wake.tv_sec++;
            }
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wake, NULL);
        }
    }

    gpiod_line_release(line_led);
    return NULL;
}

int main(int argc, char** argv) {
    const char* chip_name = (argc >= 2) ? argv[1] : "gpiochip0";

    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);

    chip = gpiod_chip_open_by_name(chip_name);
    if (!chip) {
        perror("gpiod_chip_open_by_name");
        fprintf(stderr, "힌트: sudo gpiodetect 로 사용 가능한 gpiochip 확인 후 인자로 전달하세요.\n");
        return 1;
    }

    line_trig = gpiod_chip_get_line(chip, PIN_TRIG);
    line_echo = gpiod_chip_get_line(chip, PIN_ECHO);
    line_led = gpiod_chip_get_line(chip, PIN_LED);
    if (!line_trig || !line_echo || !line_led) {
        fprintf(stderr, "GPIO 라인 획득 실패(PIN_TRIG=%d, PIN_ECHO=%d, PIN_LED=%d)\n", PIN_TRIG, PIN_ECHO, PIN_LED);
        gpiod_chip_close(chip);
        return 1;
    }

    pthread_t th1, th2;
    if (pthread_create(&th1, NULL, th_measure, NULL) != 0) {
        perror("pthread_create(th_measure)");
        gpiod_chip_close(chip);
        return 1;
    }
    if (pthread_create(&th2, NULL, th_pwm, NULL) != 0) {
        perror("pthread_create(th_pwm)");
        g_stop = 1;
        pthread_join(th1, NULL);
        gpiod_chip_close(chip);
        return 1;
    }

    pthread_join(th1, NULL);
    pthread_join(th2, NULL);

    if (chip) gpiod_chip_close(chip);
    return 0;
}