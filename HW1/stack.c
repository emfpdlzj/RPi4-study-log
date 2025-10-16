#include <stdio.h>

char stack[100];
int stackptr;

void InitStack(); //스택 초기화
void push(char x); //스택 푸시
char pop(); //스택 팝


int main(){
    InitStack();

    push('3');
    push('2');

    pop();
    pop();
    pop();


    return 0;
}

void InitStack(){
    for(int i=0;i<100;i++){
        stack[i]=0;
    }
    stackptr=0;
}
void push(char x){
    if(stackptr==100){
        printf("full !\n");
        return ;
    }
    stack[stackptr++]=x;
    printf("push : %c stackptr now :%d \n", x, stackptr);
    
        
}

char pop(){
    char ret;

    if(stackptr==0){
        printf("empty! \n");
        return '\0'; //char 반환형은 '\0'을 리턴하면 됨.
    }else{
        ret =stack[--stackptr];
        printf("pop : %c stackptr : %d \n", ret, stackptr);
    }
    
    return ret;
}

