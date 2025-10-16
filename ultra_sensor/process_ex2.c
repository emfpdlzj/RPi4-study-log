#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#define TOTALFORK 5

int main() {
    pid_t pids[TOTALFORK];
    int runProcess = 0;
    int state;
    while (runProcess < TOTALFORK) {
        pids[runProcess] = fork();
        if (pids[runProcess] < 0) {
            return -1;
        } else if (pids[runProcess] == 0) {  // child process
            printf("child process %ld is running.\n", (long)getpid());
            sleep(1);
            printf("%d process is terminated.\n", getpid());
            exit(0);
        } else {  // parent process
            // wait(&state);
            printf("parent %ld, child %d\n", (long)getpid(), pids[runProcess]);
        }
        runProcess++;
    }

    printf("parent process is terminated\n");
    return 0;
}
/*
new : 프로세스가 생성 중
running : 명령어들이 실행 중
waiting : 프로세스가 어떤 작업이 발생하기를 대기
ready : 프로세스가 프로세서에 할당되기를 대기
terminated : 프로세스의 실행 종료

*/
