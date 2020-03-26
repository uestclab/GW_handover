#ifndef __PRIORITY_QUEUE__
#define __PRIORITY_QUEUE__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define QUEUE_SIZE 1024

//#define TEST_MAIN

typedef struct Queue_item{
    int  level; // msg_number setting
    void* data; // struct msg_st*
}Queue_item;

typedef struct priorityQueue{
    pthread_mutex_t mutex;
    pthread_cond_t  wakeup;
    Queue_item **data;
    int len;
    int size;
    int start;
}priorityQueue;


priorityQueue* init_queue(int size);
int queue_destory(priorityQueue* queue);
int enPriorityQueue(void* newData, int level, priorityQueue* queue);
void* dePriorityQueue(priorityQueue* queue);


#ifdef __cplusplus
}
#endif

#endif //__PRIORITY_QUEUE__
