#ifndef GW_EVENT_PROCESS_H
#define GW_EVENT_PROCESS_H
#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>
  
#include "bs_network.h"
#include "bs_monitor.h"
#include "bs_air.h"
#include "bs_x2.h"
#include "msg_queue.h"
#include "define_common.h"
#include "gw_control.h"
#include "ThreadPool.h"

void eventLoop(g_network_para* g_network, g_air_para* g_air, g_x2_para* g_x2, 
	g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, ThreadPool* g_threadpool, zlog_category_t* zlog_handler); 



#endif//GW_EVENT_PROCESS_H


