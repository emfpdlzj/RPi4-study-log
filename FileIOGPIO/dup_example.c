#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(){
    int fd1 = open("./dup_example_data", O_WRONLY | O_CREAT, 0755);
    write(fd1, "Hello\n", strlen("Hello\n"));
    int fd2=dup(fd1);//파일 디스크립터 복제         
    write(fd2, "Hi\n", strlen("Hi\n"));
    close(fd1);
    write(fd2, "Nihao\n", strlen("Nihao\n"));
    close(fd2);
    return 0;   
}