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

// test msg type
#define MSG_NETWORK 1
#define MSG_AIR     2
#define MSG_MONITOR 3


// event
/*
	receive air signal , air event
*/
#define MSG_RECEIVED_BEACON                         4
#define MSG_RECEIVED_ASSOCIATION_RESPONSE           5
#define MSG_RECEIVED_HANDOVER_START_RESPONSE        6
#define MSG_RECEIVED_REASSOCIATION                  7

/*
	receive network signal , network event
*/
#define MSG_START_MONITOR              10
#define MSG_INIT_SELECTED              11
#define MSG_START_HANDOVER             12


#define MSG_TIMEOUT                    20

// system state 

#define STATE_STARTUP        0
#define STATE_WAIT_INIT      1
#define STATE_WORKING        2
 
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


