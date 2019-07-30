#ifndef BS_MONITOR_H
#define BS_MONITOR_H

#include "zlog.h"
#include "define_common.h"
#include "msg_queue.h"
#include "gw_utility.h"
#include "bs_network.h"
#include "gw_control.h"
#include "ThreadPool.h"

/*
typedef struct g_monitor_para{
	g_msg_queue_para*  g_msg_queue;
	g_network_para*    g_network;
	g_RegDev_para*     g_RegDev;
	ConfigureNode*     node;
	int                running;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_monitor_para;

int initMonitorThread(struct ConfigureNode* Node, g_monitor_para** g_monitor, 
		g_msg_queue_para*  g_msg_queue, g_network_para* g_network, g_RegDev_para* g_RegDev, zlog_category_t* handler);
int  freeMonitorThread(g_monitor_para* g_monitor);

void startMonitor(g_monitor_para* g_monitor, int running_step);
*/

typedef struct g_monitor_para{
	ConfigureNode*      node;
	g_RegDev_para*      g_RegDev;
	g_network_para*     g_network;
	g_msg_queue_para* 	g_msg_queue;
}g_monitor_para;

void postMonitorWorkToThreadPool(struct ConfigureNode* Node, g_msg_queue_para* g_msg_queue, g_network_para* g_network, 
								 g_RegDev_para* g_RegDev, ThreadPool* g_threadpool, int running_step);

#endif//BS_MONITOR_H
