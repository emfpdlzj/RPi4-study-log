#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
//파일에 한 줄씩 쓰고 읽는 프로그램

int main(int argc, char **argv){
    int fd;
    int rd;
    char buf[256];
    if(argc!=3){
        //argv[0] : the name of execution file  
        fprintf(stderr, "Usage: %s <filename> <access mode>\n",argv[0]);
        fprintf(stderr, "access mode is octet number, ex:0775\n");
        return 1;
    }
    fd=open(argv[1], O_WRONLY| O_CREAT | O_APPEND, 0727);
    if(fd==-1){
        perror("failed open for write");
        return 1;
    }

    write(fd, argv[2], strlen(argv[2]));
    write(fd,"\n",1);

    fd=open(argv[1],O_RDONLY, 0727);
    if(fd==-1){
        perror("failed open for read");
        return 1;
    }
    memset(buf, 0, sizeof(buf));

    while((rd=read(fd,buf, sizeof(buf))>0)){
        printf("%s\n", buf);
    }
    close(fd);
    

    return 0;
}