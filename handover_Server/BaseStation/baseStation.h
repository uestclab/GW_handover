#ifndef CLIENT_H
#define CLIENT_H
#include <pthread.h>
#include "param.h"
#include "cJSON.h"
#include "zlog.h"

typedef struct clientConfigureNode{
	char* server_ip;
	int   server_port;
}clientConfigureNode;


typedef struct para_thread{
    pthread_t*       thread_;
    pthread_mutex_t* mutex_;
    pthread_cond_t*  cond_;
}para_thread;

typedef struct GW_buf{
	void* buf_;
	int length_buf;
}GW_buf;

#define WAIT_MSG "press enter 'g' to continue..."
#define BUFFER_SIZE 1024 * 40


int32_t myNtohl(const char* buf);
void  user_wait();
char* readfile(const char *path);


// callback , register callback function
typedef int (*frcb_func)(const char* jsonStr, void* rptr, void* out); //out = GW_buf

typedef struct mydata {
    const char* jsonStr; 
    frcb_func frcb;
	void* rptr;
} mydata_t;

int registerCallbackFunc(frcb_func read_fcb, void* rptr);
void releaseCallbackFunc();


// send message --- inform event actively 
void informServer(eventID event, char* param);
int  subscriber_event(eventID event);
int  unsubscriber(eventID event);
frameInfo* clear_frameInfo();
void getFromDevice(deviceID dest, char* param);
void writeToDevice(deviceID dest, char* param);

// receive message
void* receive_thread(void* args);
void  receive();
frameInfo*  parseFrame(const char* buf, int32_t length);
void  proReceiveMessage(const char* buf, int32_t length);

int  initThread(deviceID device, zlog_category_t* log_handler);
int  freeThread();


// messageLength vacancy, if info then free frame_buf;
void assembleFrame();


#endif//CLIENT_H











