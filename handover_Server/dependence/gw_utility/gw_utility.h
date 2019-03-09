#ifndef GW_UTILITY_H
#define GW_UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>


typedef struct para_thread{
    pthread_t*       thread_pid;
    pthread_mutex_t* mutex_;
    pthread_cond_t*  cond_;
}para_thread;

para_thread* newThreadPara();
void destoryThreadPara(para_thread* para_t);

int32_t myNtohl(const char* buf);
int filelength(FILE *fp);
char* readfile(const char *path);
int64_t now();

void hexdump(const void* p, size_t size);
int get_mac(char * mac, int len_limit, char *arg);
void change_mac_buf(char* in_addr, char* out_addr);
void reverseBuf(char* in_buf, char* out_buf, int number);

// -- temp 
void user_wait();

#ifdef __cplusplus
}
#endif

#endif//GW_UTILITY_H
