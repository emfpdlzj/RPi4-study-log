#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(){
    int fd1=0, fd2=0, fd3=0;

    fd1=open("./fcntl_example_data", O_WRONLY|O_CREAT|O_TRUNC);
    printf("fcntl(F_DUPFD) TEST \n");
    fd2= fcntl(fd1, F_DUPFD, 30);
    printf("fd2 = %d\n", fd2); 
    //fcntl()로 30이상의 fd를 할당하여 fd2, fd3에 반환

    fd3=fcntl(fd1, F_DUPFD, 30);
    printf("fd3= %d\n", fd3);

    close(fd2);
    close(fd3);

    printf("dup2() TEST \n");
    //dup2로 30의 fd를 할당하여 fd2,fd3에 반환
    fd2 = dup2(fd1, 30);
    printf("fd2 = %d\n", fd2);

    fd3 = dup2(fd1, 30);
    printf("fd3 = %d\n", fd3);

    close(fd2);
    close(fd3);

    close(fd1);
    return 0;

}