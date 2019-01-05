#ifndef BASESTATION_TR_ARM_H
#define BASESTATION_TR_ARM_H
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "define_common.h"
#define BUFFER_SIZE 1024 * 40

typedef struct para_thread{
    pthread_t*       thread_;
    pthread_mutex_t* mutex_;
    pthread_cond_t*  cond_;
}para_thread;

#define WAIT_MSG "press enter 'g' to continue..."


int32_t myNtohl(const char* buf);
void  user_wait();
char* readfile(const char *path);

// send message
signal_json* clear_json();
void sendSignal(signalType type, signal_json* json);

// receive message
void  receive();
void processMessage(char* buf, int32_t length);
messageInfo* parseMessage(char* buf, int32_t length);


void* receive_thread(void* args);
int  initThread(struct ConfigureNode* Node, zlog_category_t* log_handler);
int  freeThread();


#endif//CLIENT_H











