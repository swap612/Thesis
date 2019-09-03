/**
 * @file matrixSignal.c
 * @author Swapnil Raykar (swap612@gmail.com)
 * @brief Program to perform simple matrix operation with signal Handler
 * @version 0.1
 * @date 2019-08-23
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include<time.h>

int NUM_ELE = 2 * 1024 * 1024;
int *matrixA, *matrixB;
static int times = 10000;
   
// Signal Handler
void sigHandler(int sig)
{
    time_t my_time = time(NULL); 
  
// ctime() used to give the present time 
printf("%s", ctime(&my_time));  
    // static int flag = 0;
    // flag = !flag;
    for (int i = 0; i < NUM_ELE; i++)
        matrixA[i] = i * 44;
    printf("Signal REceived");
    while(times--)
    {
        for (int i = 0; i < NUM_ELE; i++)
            matrixA[i];// = i * 2;
    }
    time_t my_time1 = time(NULL); 
  
// ctime() used to give the present time 
printf("%s", ctime(&my_time1)); 
    printf("SigHandler return");
    exit(0);
}

int main(int argc, char **argv)
{

    matrixA = (int *)malloc(NUM_ELE * sizeof(int));
    // matrixB = (int *)malloc(NUM_ELE * sizeof(int));

    // Initialize
    
    // for (int i = 0; i < NUM_ELE; i++)
    //     matrixB[i] = i * 40;

    // Register the SIGINT signal
    signal(SIGINT, &sigHandler);
    
    while(1);   // waiting for signals
    
    /* Sending a SIGINT tyhrough kill cmd
        pid_t myPid = getpid();
        kill(myPid, SIGINT);
    */

    return 0;
}
