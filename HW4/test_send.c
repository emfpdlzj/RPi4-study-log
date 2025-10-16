#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/dev/gpiomorse", O_WRONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    char input[256];
    printf("Type message to send (alphabets only, 'exit' to quit):\n");

    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("Input error\n");
            break;
        }
        // 개행 문자 제거
        input[strcspn(input, "\n")] = 0;
        if (strcmp(input, "exit") == 0) //exit:종료 
            break;

        ssize_t ret = write(fd, input, strlen(input));
        if (ret < 0) {
            perror("write");
            break;
        }
        printf("Sent: %s\n", input);
    }

    close(fd);
    return 0;
}
