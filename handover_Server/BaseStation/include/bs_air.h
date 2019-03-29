#ifndef BS_AIR_H
#define BS_AIR_H

#include "zlog.h"
#include "gw_utility.h"
#include "msg_queue.h"

typedef struct g_air_para{
	g_msg_queue_para*  g_msg_queue;
	int                running;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_air_para;

int initProcessAirThread(struct ConfigureNode* Node, g_air_para** g_air, g_msg_queue_para*  g_msg_queue, zlog_category_t* handler);
int  freeProcessAirThread(g_air_para* g_air);

void startProcessAir(g_air_para* g_air, int running_step);

#endif//BS_AIR_H
