#ifndef BS_X2_H
#define BS_X2_H


#include "zlog.h"
#include "define_common.h"
#include "msg_queue.h"
#include "gw_utility.h"

#define UDP_BUFFER_SIZE 2048 

typedef struct g_x2_para{
	g_msg_queue_para*  g_msg_queue;
	ConfigureNode*     node;
	para_thread*       para_t;
	int                connected;
	int                sock_server;
	int                sock_client;
	char*              remote_ip;
	char*              receive_buf;
	zlog_category_t*   log_handler;
}g_x2_para;


void configure_target_ip(char* ip, g_x2_para* g_x2);

void sendX2Signal(char* json, g_x2_para* g_x2);

int  initX2Thread(struct ConfigureNode* Node, g_x2_para** g_x2, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
int  freeX2Thread(g_x2_para* g_x2);


#endif//BS_X2_H











