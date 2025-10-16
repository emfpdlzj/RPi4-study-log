#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int fd = open("./dup2_example_data", O_WRONLY | O_CREAT, 0755);

    if (dup2(fd, STDOUT_FILENO) == -1)
    {
        perror("failed dup2");
        exit(1);
    }

    printf("Hello World!\n");
    close(fd);
    return 0;
}