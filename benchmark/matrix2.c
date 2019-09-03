/**
 * @file matrix.c
 * @author Swapnil Raykar (swap612@gmail.com)
 * @brief Program to perform simple matrix operations
 * @version 0.1
 * @date 2019-08-21
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int times = 10000;
int NUM_ELE = 3 * 1024 * 1024;
int main(int argc, char **argv)
{

    // int *matrixB = (int *)malloc(NUM_ELE * sizeof(int));
    // printf("slepping.. Set the PQoS\n");
    // sleep(2);
    // printf("WakeUp\n");
    int *matrixA = (int *)malloc(NUM_ELE * sizeof(int));

    while(times--)
    {
        for (int i = 0; i < NUM_ELE; i++)
            matrixA[i] = i * 44;

        // for (int i = 0; i < NUM_ELE; i++)
        //     matrixB[i] = i * 40;
    }

    return 0;
}
