#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
// lseek은 파일포인터를 변경하기 위해 사용함.
int main(int argc, char **argv)
{
    int fd;
    int i, buf;
    fd = open("num.dat", O_RDWR | O_CREAT); // 파일 열기

    if (fd < 0)
    {
        perror("error: ");
        exit(0);
    }

    for (int i = 1000; i < 1010; i++)
    { // 10개의 문자쓰기
        write(fd, (void *)&i, sizeof(i));
    }

    lseek(fd, 0, SEEK_SET);
    read(fd, (void *)&buf, sizeof(buf)); // 파일 시작점에서 0만큼 포인터 이동 후 읽기
    printf("%d \n", buf);

    lseek(fd, 4 * sizeof(i), SEEK_SET); // 파일 시작점에서 4만큼 포인터 이동 후 읽기
    read(fd, (void *)&buf, sizeof(buf));
    printf("%d\n", buf);
}