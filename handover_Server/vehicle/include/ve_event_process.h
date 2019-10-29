#ifndef GW_VE_EVENT_PROCESS_H
#define GW_VE_EVENT_PROCESS_H
#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>
  
#include "ve_air.h"
#include "ve_periodic.h"
#include "msg_queue.h"
#include "common.h"
#include "gw_control.h"
#include "ThreadPool.h"

void eventLoop(g_air_para* g_air, g_periodic_para* g_periodic, g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, 
			ThreadPool* g_threadpool, zlog_category_t* zlog_handler);


#endif//GW_VE_EVENT_PROCESS_H


