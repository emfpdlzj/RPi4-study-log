## libpiod

헤더 ``` #include <gpiod.h> ```

### 기본 사용
- GPIO 칩 열기 :  ```gpiod_chip_open()```
- 라인(GPIO 핀) 가져오기 ```gpiod_chip_get_line()```
- 입출력 요청
```gpiod_line_request_output(), gpiod_line_request_input()```
- 값 읽기/쓰기
```gpiod_line_get_value(), gpiod_line_set_valu()```
- 해제 ```gpiod_chip_close()```

### 예시코드
```
int main() {
    const char *chipname = "/dev/gpiochip0";
    int line_num = 17; // BCM 17번 핀

    struct gpiod_chip *chip = gpiod_chip_open(chipname);
    struct gpiod_line *line = gpiod_chip_get_line(chip, line_num);

    gpiod_line_request_output(line, "my-app", 0); // 초기값 LOW
    gpiod_line_set_value(line, 1); // HIGH 출력

    gpiod_chip_close(chip);
    return 0;
}
```     