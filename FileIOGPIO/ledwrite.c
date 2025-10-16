/*리눅스에서 gpio 제어를 통해 외부장치를 깜빡이게 하는 프로그램.*/
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //sysfs 방식으로 gpio 제어
#include <sys/types.h>
#include <unistd.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1 //high,low를 반복해서 깜빡임 구현
#define POUT 17 //gpio 17번 핀을 출력핀으로 설정함.
#define VALUE_MAX 40 //value: read, write을 통해 해당 gpio에 값을 write,read gka.
#define DIRECTION_MAX 128

static int GPIOExport(int pin);
static int GPIODirection(int pin, int dir);
static int GPIOWrite(int pin, int value);
static int GPIOUnexport(int pin);

int main(int argc, char* argv[]){
    int repeat=10; //10번 반복

    if (GPIOExport(POUT)==-1){ //커널에 GPIO의 제어권을 이 파일에 기록하여 사용자 공간으로 내보내도록 요청
        return 1;
    }

    if(GPIODirection(POUT,OUT)==-1){//해당 gpio의 입력, 출력 결정
        return 2;
    }

    do{
        if(GPIOWrite(POUT, repeat % 2) == -1){
            return 3;
        }
        usleep(500*1000); //0.5초 대기함.   
    }while(repeat--);

    if(GPIOUnexport(POUT)==-1){ // gpio의 제어권 반납. 오류 발생시 생략 가능.
        return 4;
    }

    return 0;
}

//gpio핀 번호를 전달받아 해당 핀을 export해서 유저 공간에서 제어할 수 있게 함

static int GPIOExport(int pin){
    #define BUFFER_MAX 3
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd=open("/sys/class/gpio/export", O_WRONLY); //gpio제어용 파일을 wronly로 연다.
    if(-1 == fd){
        fprintf(stderr, "Failed to open export for writing!\n");
        return -1;
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin); // gpio핀 번호를 문자열로 바꿔 buffer에 저장
    //%d는 정수를 문자열로 변환하는 포맷 지정자
    write(fd, buffer, bytes_written);//그 문자열을 해당 폴더에 씀 > export 완료함.
    close(fd);
    return (0);
}

static int GPIODirection(int pin, int dir){
    static const char s_directions_str[] = "in\0out";
    //문자열 배열 생성 : i, n \0, o, u, t, \0

    char path[DIRECTION_MAX];
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin); //경로생성
    fd = open(path, O_WRONLY);//생성한 경로로 fd 생성, 방향은 wr
    if(-1==fd){ //실패시 출력
        fprintf(stderr, "Failed to open gpio direction for writing! \n");
        return (-1);
    }

    if  (-1 == write(fd, &s_directions_str[IN == dir ? 0:3], IN==dir ? 2:3)){
        //fd가 in이면 0, Out이면 3, in이면 길이 2, Out이면 길이 3
        fprintf(stderr, "Failed to set direction!\n");
        return (-1);
    }

    close(fd);
    return (0);
    //snprintf는 Formatted strin을 버퍼에 저장하는 함수이다.
}

static int GPIOWrite(int pin, int value){ //gpio에 특정값을 write
    static const char s_values_str[] ="01";
    char path[VALUE_MAX];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio//gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if(-1==fd){
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return (-1);
    } 
    if(1!=write(fd, &s_values_str[LOW==value ? 0:1], 1)){
        //low면 0, high면 1 write.
        fprintf(stderr, "Failed to write value! \n");
        return (-1);
    }
    close(fd);
    return (0);
}
static int GPIOUnexport (int pin){
    //unexport에 gpio pin number write

    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd=open("/sys/class/gpio/unexport", O_WRONLY);
    if(-1==fd){
        fprintf(stderr, "Faile to open unexport for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return (0);
}