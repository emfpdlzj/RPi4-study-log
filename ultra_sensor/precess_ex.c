#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void main(void)
{
    pid_t pid;
    pid = fork();   		//make child process
    if(pid == -1) {
        printf("can't fork\n");
        exit(0);
    }
    if(pid == 0) {
        printf("child process id: %d\n", getpid());
        sleep(1);
        exit(0);
    }

    else {
        printf("parent process id: %d\n", getpid());
        sleep(1);
        exit(1);
    }
    return;
}
/*Process 간 서로 다른 메모리 사용(메모리 공유 x)
parent fork()값 = child process의 getpid() 값
child proces의 fork()값 = 0
리눅스는 부팅할 때, BIOS대로 부팅을 하고, GRUB에서 MBR(마스터 부트 레코드?)라는 것을 로드하고, 커널 적재하고, 
파일 시스템 마운트하고, init process로 시스템 초기화를 한다.
커널은 PID가 0이고, init 프로세스는 PID가 1이다. 
kernel process에서부터 계속 fork를 하는 방식으로 프로세스를 만든다는 것이다. 커널은 유일하게 부모가 없다…
이는 ps –efj –U root로 알 수 있다.
*/