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

#include "gw_utility.h" 

#define MAX_TEXT 2048

struct msg_st  
{  
    long int msg_type;
	int      msg_number;
	int      msg_len;  
    char     msg_json[MAX_TEXT];  
};

typedef struct g_msg_queue_para{
	int                msgid;
	para_thread*       para_t;
	int                seq_id;
	zlog_category_t*   log_handler;
}g_msg_queue_para;


g_msg_queue_para* createMsgQueue(const char *pathname, int proj_id, zlog_category_t* handler);
int delMsgQueue(g_msg_queue_para* g_msg_queue);

void postMsgQueue(struct msg_st* data, g_msg_queue_para* g_msg_queue);
struct msg_st* getMsgQueue(g_msg_queue_para* g_msg_queue);

int clearMsgQueue(g_msg_queue_para* g_msg_queue);

#endif//GW_MSG_QUEUE_H


