#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define TRIG_PIN 23  // GPIO23
#define ECHO_PIN 24  // GPIO24

#define CHIP_NAME "gpiochip0"

int main() {
    struct gpiod_chip *chip;
    struct gpiod_line *trig_line, *echo_line;

    chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        perror("Open chip failed");
        return 1;
    }

    // Trig 출력용, Echo 입력용 라인 요청
    trig_line = gpiod_chip_get_line(chip, TRIG_PIN);
    echo_line = gpiod_chip_get_line(chip, ECHO_PIN);
    if (!trig_line || !echo_line) {
        perror("Get line failed");
        return 2;
    }

    // Trig는 출력
    if (gpiod_line_request_output(trig_line, "ultra", 0) < 0) {
        perror("Trig request failed");
        return 3;
    }

    // Echo는 입력
    if (gpiod_line_request_input(echo_line, "ultra") < 0) {
        perror("Echo request failed");
        return 4;
    }

    for (int repeat = 0; repeat < 10; repeat++) {
        struct timespec start, end;
        double duration;

        // Trig LOW → HIGH 10us → LOW
        gpiod_line_set_value(trig_line, 0);
        usleep(2);
        gpiod_line_set_value(trig_line, 1);
        usleep(10);  // 10us pulse
        gpiod_line_set_value(trig_line, 0);

        // Echo rising edge 대기
        while (gpiod_line_get_value(echo_line) == 0) {
            clock_gettime(CLOCK_MONOTONIC, &start);
        }

        // Echo falling edge 대기
        while (gpiod_line_get_value(echo_line) == 1) {
            clock_gettime(CLOCK_MONOTONIC, &end);
        }

        // 시간 계산 (초 + 나노초 → 초 단위 double)
        duration = (end.tv_sec - start.tv_sec) +
                   (end.tv_nsec - start.tv_nsec) / 1e9;

        double distance = duration * 17000;  // 왕복 거리 → 편도 = 속도 * 시간 / 2
        printf("Distance: %.2f cm\n", distance);

        usleep(500000);  // 0.5초마다 측정
    }

    // 정리
    gpiod_line_release(trig_line);
    gpiod_line_release(echo_line);
    gpiod_chip_close(chip);

    return 0;
}
