/* Task 1: Write a program to read/write registers using inline assembly */

#include<stdio.h>
#include<stdlib.h>

int main(int argc, char ** argv){

    printf("*** Welcome to Swapnil-lab ***\n");
     int a =22, b;
    printf("Before asm a:%d b:%d \n", a,b);

    __asm__ __volatile__ ("movl %1, %%eax ;\n\t "
            "movl %%eax, %0 ;"
          :"=r"(b)
          :"r"(a)
          :"%eax"
    );
    
    printf("After asm a:%d b:%d \n", a,b);
    
    return 0;
}