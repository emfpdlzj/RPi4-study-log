#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

int fd;  
void sigio_handler(int sig) { 
    char buf[256];
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("Received: %s\n", buf);
        if (strncmp(buf, "exit", 4) == 0) {
            close(fd);
            printf("Exiting.\n");
            _exit(0);  
        }
    }
}

int main() {
    fd = open("/dev/gpiomorse", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    //시그널 관련 
    signal(SIGIO, sigio_handler);
    if (fcntl(fd, F_SETOWN, getpid()) == -1) {
        perror("fcntl - F_SETOWN");
        return 1;
    }

    int flags = fcntl(fd, F_GETFL);
    if (fcntl(fd, F_SETFL, flags | O_ASYNC) == -1) {
        perror("fcntl - O_ASYNC");
        return 1;
    }

    printf("Waiting for incoming messages (SIGIO)...\n");

    while (1) {
        pause();   //시그널 받을 때 까지 대기
    }

    return 0;
}
