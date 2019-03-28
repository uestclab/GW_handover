#ifndef BS_MONITOR_H
#define BS_MONITOR_H
#include <pthread.h>
#include "cJSON.h"
#include "zlog.h"
#include "define_common.h"

#include "gw_utility.h"

typedef struct g_monitor_para{
	int                running;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_monitor_para;

int initMonitorThread(struct ConfigureNode* Node, g_monitor_para** g_monitor, zlog_category_t* handler);
int  freeMonitorThread(zlog_category_t* handler);

void startMonitor(g_monitor_para* g_monitor);

#endif//BS_MONITOR_H
