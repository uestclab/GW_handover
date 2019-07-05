#ifndef BS_NETWORK_H
#define BS_NETWORK_H

#include "zlog.h"
#include "define_common.h"
#include "msg_queue.h"
#include "gw_utility.h"

#define BUFFER_SIZE 1024 * 40

typedef struct g_network_para{
	g_msg_queue_para*  g_msg_queue;
	ConfigureNode*     node;
	para_thread*       para_t;
	int                sock_cli;
	int                connected;
	int                gMoreData_;
	char*              sendMessage;
	char*              recvbuf;
	int                startup; // first signal(id_pair) to server !
	zlog_category_t*   log_handler;
}g_network_para;


// send message
void sendSignal(signalType type, char* json, g_network_para* g_network);

// receive message
void receive(g_network_para* g_network);
void processMessage(char* buf, int32_t length, g_network_para* g_network);
messageInfo* parseMessage(char* buf, int32_t length);

int  initNetworkThread(struct ConfigureNode* Node, g_network_para** g_network, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
int  freeNetworkThread(g_network_para* g_network);


#endif//BS_NETWORK_H











