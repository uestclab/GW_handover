#ifndef GW_MSG_QUEUE_H
#define GW_MSG_QUEUE_H
#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>  
#include <sys/msg.h> 
#include <ctype.h>
#include "cJSON.h"
#include "zlog.h"

#include "define_common.h"
#include "gw_utility.h" 

#define MAX_TEXT 200
#define MSG_NETWORK 1
#define MSG_AIR     2
#define MSG_MONITOR 3
 
struct msg_st  
{  
    long int msg_type;
	int      msg_number;  
    char     json[MAX_TEXT];  
};


typedef struct g_msg_queue_para{
	int                msgid;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_msg_queue_para;


g_msg_queue_para* createMsgQueue(struct ConfigureNode* Node, zlog_category_t* handler);
int delMsgQueue(g_msg_queue_para* g_msg_queue);

void enqueue(struct msg_st* data, g_msg_queue_para* g_msg_queue);
struct msg_st* getMsgQueue(g_msg_queue_para* g_msg_queue);


#endif//GW_MSG_QUEUE_H


