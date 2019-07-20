#ifndef BS_AIR_H
#define BS_AIR_H

#include "zlog.h"
#include "gw_utility.h"
#include "msg_queue.h"
#include "define_common.h"

typedef struct g_air_para{
	g_msg_queue_para*  g_msg_queue;
	int                running;
    ConfigureNode*     node;
	para_thread*       para_t;
	para_thread*       send_para_t;
	zlog_category_t*   log_handler;
}g_air_para;

int initProcessAirThread(struct ConfigureNode* Node, g_air_para** g_air, 
		g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
int freeProcessAirThread(g_air_para* g_air);

void startProcessAir(g_air_para* g_air, int running_step);

int send_airSignal(int32_t subtype, char* mac_buf, char* mac_buf_dest, char* mac_buf_next, g_air_para* g_air);

#endif//BS_AIR_H
