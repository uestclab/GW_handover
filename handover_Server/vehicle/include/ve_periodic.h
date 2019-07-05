#ifndef VE_PERIODIC_H
#define VE_PERIODIC_H

#include "zlog.h"
#include "common.h"
#include "msg_queue.h"
#include "gw_utility.h"
#include "ve_air.h"


typedef struct g_periodic_para{
	g_msg_queue_para*  g_msg_queue;
	ConfigureNode*     node;
	g_air_para*    	   g_air;
	int                running;
	para_thread*       para_t;
	para_thread*       para_check_t;
	zlog_category_t*   log_handler;
}g_periodic_para;

int initPeriodicThread(struct ConfigureNode* Node, g_periodic_para** g_periodic, g_air_para* g_air, 
			g_msg_queue_para*  g_msg_queue, zlog_category_t* handler);
int  freePeriodicThread(g_periodic_para* g_periodic);

void startPeriodic(g_periodic_para* g_periodic, int running_step);
void stopPeriodic(g_periodic_para* g_periodic);


#endif//VE_PERIODIC_H
