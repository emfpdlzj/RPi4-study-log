# libpiod

공부동기: JETPACK6부터 GPIO sysfs가 linux 4.8 Linux 커널부터 더 이상 사용되지 않기 때문에 sysfs 방식이 사용이 불가능해졌다. 나는 sysfs로 제어하는 방법밖에 모르기때문에, 계속해서 라즈베리파이를 연구하려면 다른 방법이 필요했다.
 libgpiod는 내가 sysfs제어에 사용하던 것 처럼 c언어에서 제어가능하다. 또한 커널에서 계속 관리중이고 연구/논문/예제 가 많다는 이점이 있어서 선택했다.

참고링크: 
- https://rorsi.tistory.com/144
- https://libgpiod.readthedocs.io/en/latest/core_line_settings.html


mission.c에선 아래와 같이 구현해봤다.

>초음파센서 & LED 제어  
측정 거리가 작을수록 LED가 밝아지고 클수록 어두워지는 예제  
전역 변수 distance를 공유하는 데이터 공간으로 사용

sysfs 방식이 막혔기 때문에, 이전 과제에서 진행했던 /sys/class/pwm/에 write하는 함수들은 다 교체해야 했다. 대신 libgpiod로 LED 핀을 직접 토글하면서 duty 계산, nsleep 로직으로 PWM을 구현했다. 