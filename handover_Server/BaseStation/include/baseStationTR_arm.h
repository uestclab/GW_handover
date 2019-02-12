#ifndef BASESTATION_TR_ARM_H
#define BASESTATION_TR_ARM_H
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "define_common.h"
#define BUFFER_SIZE 1024 * 40

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











