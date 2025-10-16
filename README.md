# 🧠 RPi4 Study Log

라즈베리파이4(RPi4) 기반 임베디드/시스템 프로그래밍 학습 저장소.  
MIPS 어셈블리 → 파일 I/O & GPIO → 센서/스레드 → 디바이스 드라이버까지 단계별 실습과 과제를 기록합니다.

---

## 📁 Repository Structure

```

Rpi4-study-log/
┣ 📂 Embedded interface      # LED 제어 & SPI 통신 실습
┣ 📂 FileIOGPIO              # 파일 입출력 & GPIO 제어 실습
┣ 📂 ultra_sensor            # 초음파 센서 + pthread/프로세스 실습
┣ 📂 HW1_MIPS_Assembly       # HW1: 밉스 어셈블리어로 구현
┣ 📂 HW1.5_Linker_Loader     # HW1.5: 링커·로더 퀴즈/정리
┣ 📂 HW2_Kernel_Debate       # HW2: 토발즈 vs 타넨바움 논쟁 분석
┣ 📂 HW3_Button_LED_Sensor   # HW3: 버튼 입력 기반 LED/센서 제어
┣ 📂 HW4_Device_Driver       # HW4: GPIO 디바이스 드라이버 만들기
┗ 📄 README.md

````
## 💻 Dev Environment

- **Board**: Raspberry Pi 4 Model B  
- **OS**: Raspberry Pi OS 
- **Lang/Tools**: C, GCC/Make, VSCode, `libgpiod` 
- **Kernel**: 모듈 빌드/로딩, sysfs 인터페이스 실습

---

## 📚 Reference

- *Linux Device Drivers, 3rd* / Tanenbaum OS  
- Raspberry Pi Docs /  강의 자료 

---
