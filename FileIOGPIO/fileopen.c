#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv){
    int fd=0;
    if(argc!=3){
        fprintf(stderr, "Usage:%s <file name> <access mode> \n",argv[0]);
        fprintf(stderr,"access mode is octet number, ex:0775 \n");
        return 1;
    }
    
    mode_t mode=0;
    //argv[2]: second prarameter of open function
    sscanf(argv[2],"0%o", &mode);
    fd=open(argv[0], O_WRONLY | O_CREAT |O_EXCL, mode);
    if(fd==-1){
        printf("open failed.\n");
        return 1;
    }

    //argv[1]:first parameter of open function  
    printf("success oepn %s\n",argv[1]);
    close (fd) ;


    return 0;
}