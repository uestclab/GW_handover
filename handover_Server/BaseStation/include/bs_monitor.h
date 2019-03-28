#ifndef BS_MONITOR_H
#define BS_MONITOR_H
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "define_common.h"

#include "gw_utility.h"
#include "msg_queue.h"

typedef struct g_monitor_para{
	g_msg_queue_para*  g_msg_queue;
	int                running;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_monitor_para;

int initMonitorThread(struct ConfigureNode* Node, g_monitor_para** g_monitor, g_msg_queue_para*  g_msg_queue, zlog_category_t* handler);
int  freeMonitorThread(g_monitor_para* g_monitor);

void startMonitor(g_monitor_para* g_monitor);

#endif//BS_MONITOR_H
