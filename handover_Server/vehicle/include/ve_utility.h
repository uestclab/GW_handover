#ifndef VE_UTILITY_H
#define VE_UTILITY_H

#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>

#include "ve_air.h"
#include "gw_control.h"
#include "gw_frame.h"
#include "msg_queue.h"
#include "common.h"
#include "ThreadPool.h"

typedef struct retrans_air_t{
	int32_t subtype;
	g_msg_queue_para* g_msg_queue;
	g_air_para*       g_air;
}retrans_air_t;

void checkReceivedList(int32_t subtype, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_air_para* g_air, ThreadPool* g_threadpool);

void postCheckWorkToThreadPool(int32_t subtype, g_msg_queue_para* g_msg_queue, g_air_para* g_air, ThreadPool* g_threadpool);















#endif//VE_UTILITY_H
