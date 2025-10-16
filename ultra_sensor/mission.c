#include <stdio.h>      // printf, perror 등 표준 입출력 함수
#include <stdlib.h>     // exit 등 유틸리티 함수
#include <unistd.h>     // usleep 등 POSIX 함수
#include <pthread.h>    // pthread_create, pthread_join 등 쓰레드 함수
#include <time.h>       // clock_gettime, nanosleep 등 시간 관련 함수
#include <gpiod.h>      // libgpiod GPIO 제어 함수

// GPIO 핀 번호 정의 (BCM 기준)
#define TRIG_PIN 23     // 초음파 센서 TRIG 핀
#define ECHO_PIN 24     // 초음파 센서 ECHO 핀
#define LED_PIN 18      // LED 연결 핀

#define CHIP_NAME "gpiochip0"      // 사용할 GPIO 칩 이름
#define CONSUMER "ultrasonic_pwm"  // GPIO 소비자 이름 (로그 등에서 사용)

// 공유 거리 변수 (두 스레드 간 공유됨)
volatile double distance = 0.0;

// GPIO 핸들 전역 변수
struct gpiod_chip *chip;
struct gpiod_line *trig, *echo, *led;

// 마이크로초 단위 지연 함수
void delay_us(int us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);  // 정밀한 us 단위 대기
}

// 거리 측정 스레드 함수
void* measure_distance(void* arg) {
    while (1) {
        // TRIG 핀에 10us 펄스를 보냄 (초음파 송신)
        gpiod_line_set_value(trig, 0);
        delay_us(2);
        gpiod_line_set_value(trig, 1);
        delay_us(10);
        gpiod_line_set_value(trig, 0);

        // ECHO 핀이 HIGH 될 때까지 대기 (초음파 수신 시작)
        while (gpiod_line_get_value(echo) == 0);

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);  // 수신 시작 시간 기록

        // ECHO 핀이 LOW 될 때까지 대기 (초음파 수신 끝)
        while (gpiod_line_get_value(echo) == 1);
        clock_gettime(CLOCK_MONOTONIC, &end);    // 수신 종료 시간 기록

        // 시간 차이 계산 (마이크로초 단위)
        double duration = (end.tv_sec - start.tv_sec) * 1e6 +
                          (end.tv_nsec - start.tv_nsec) / 1000.0;

        // 거리 계산: 음속을 이용하여 거리(cm)로 변환
        distance = duration / 58.0;

        usleep(100000);  // 100ms 간격으로 반복
    }

    return NULL;
}

// LED 밝기 조절 스레드 함수 -PWM
void* led_pwm_control(void* arg) {
    while (1) {
        double dist = distance;  // 공유 변수 읽기

        // 거리(0~50cm)에 따라 듀티비 조절 (가까울수록 밝게)
        int duty = 0;
        if (dist < 2.0)
            duty = 100;         // 아주 가까우면 100% 밝기
        else if (dist > 50.0)
            duty = 0;           // 멀면 꺼짐
        else
            duty = (int)(100.0 - (dist / 50.0) * 100.0);  // 선형 보간

        int period_us = 10000;  // PWM 주기: 10ms (100Hz)
        int on_time = (period_us * duty) / 100;     // LED ON 시간
        int off_time = period_us - on_time;         // LED OFF 시간

        gpiod_line_set_value(led, 1);   // ON
        delay_us(on_time);
        gpiod_line_set_value(led, 0);   // OFF
        delay_us(off_time);
    }

    return NULL;
}

int main() {
    // GPIO 칩 열기
    chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        perror("gpiod_chip_open_by_name");
        exit(EXIT_FAILURE);
    }

    // 핀별 GPIO 라인 요청
    trig = gpiod_chip_get_line(chip, TRIG_PIN);
    echo = gpiod_chip_get_line(chip, ECHO_PIN);
    led  = gpiod_chip_get_line(chip, LED_PIN);

    // 핀 할당 실패 시 종료
    if (!trig || !echo || !led) {
        fprintf(stderr, "GPIO line request error\n");
        exit(EXIT_FAILURE);
    }

    // 핀 방향 설정
    gpiod_line_request_output(trig, CONSUMER, 0);  // 출력: TRIG
    gpiod_line_request_input(echo, CONSUMER);      // 입력: ECHO
    gpiod_line_request_output(led, CONSUMER, 0);   // 출력: LED

    // 거리 측정 스레드 및 LED 제어 스레드 생성
    pthread_t t1, t2;
    pthread_create(&t1, NULL, measure_distance, NULL);
    pthread_create(&t2, NULL, led_pwm_control, NULL);

    // 메인 스레드는 두 스레드가 종료될 때까지 대기
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // 종료 시 리소스 해제
    gpiod_line_release(trig);
    gpiod_line_release(echo);
    gpiod_line_release(led);
    gpiod_chip_close(chip);

    return 0;
}
