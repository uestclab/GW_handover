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

#include "common.h"
#include "gw_utility.h" 

#define MAX_TEXT 200

// test msg type
#define MSG_NETWORK 1
#define MSG_AIR     2
#define MSG_MONITOR 3


// system state 

#define STATE_STARTUP           0
#define STATE_SYSTEM_READY      1
#define STATE_WORKING           2
#define STATE_HANDOVER          3


#define MSG_RECEIVED_ASSOCIATION_REQUEST      10
#define MSG_RECEIVED_DEASSOCIATION            11
#define MSG_RECEIVED_HANDOVER_START_REQUEST   12
 
#define MSG_STARTUP             20

struct msg_st  
{  
    long int msg_type;
	int      msg_number;  
    char     msg_json[MAX_TEXT];  
};


typedef struct g_msg_queue_para{
	int                msgid;
	para_thread*       para_t;
	int                seq_id;
	zlog_category_t*   log_handler;
}g_msg_queue_para;


g_msg_queue_para* createMsgQueue(struct ConfigureNode* Node, zlog_category_t* handler);
int delMsgQueue(g_msg_queue_para* g_msg_queue);

void postMsgQueue(struct msg_st* data, g_msg_queue_para* g_msg_queue);
struct msg_st* getMsgQueue(g_msg_queue_para* g_msg_queue);


#endif//GW_MSG_QUEUE_H


