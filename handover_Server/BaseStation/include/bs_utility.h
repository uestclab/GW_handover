#ifndef BS_UTILITY_H
#define BS_UTILITY_H

#include <stdio.h>  
#include <pthread.h>  
#include <stdlib.h>  
#include <unistd.h>  
#include <signal.h>   
#include <string.h>  
#include <errno.h>

#include "bs_air.h"
#include "gw_control.h"
#include "gw_frame.h"
#include "msg_queue.h"
#include "define_common.h"
#include "ThreadPool.h"

typedef struct retrans_air_t{
	int32_t subtype;
	g_msg_queue_para* g_msg_queue;
}retrans_air_t;

typedef struct g_test_para{
	ConfigureNode*      node;
	g_RegDev_para* 		g_RegDev;
	g_msg_queue_para* 	g_msg_queue;
}g_test_para;

void timer_cb(void* in_data, g_msg_queue_para* g_msg_queue);

int is_my_air_frame(char* src, char* dest);

void configureDstMacToBB(char* dst_buf, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler);

struct net_data* parseNetMsg(struct msg_st* getData);

void printMsgType(long int type);

struct air_data* parseAirMsg(char* msg_json);

/* duplicate receive and retransmit air frame */

int checkAirFrameDuplicate(msg_event receivedAirEvent, system_info_para* g_system_info);

void resetReceivedList(received_state_list* list);

void postCheckSendSignal(int32_t subtype, g_msg_queue_para* g_msg_queue);

void checkReceivedList(int32_t subtype, system_info_para* g_system_info, g_msg_queue_para* g_msg_queue, 
					  g_air_para* g_air, ThreadPool* g_threadpool);

void* retrans_air_process_thread(void* arg);

void postCheckWorkToThreadPool(int32_t subtype, g_msg_queue_para* g_msg_queue, ThreadPool* g_threadpool);


void postCheckTxBufferWorkToThreadPool(struct ConfigureNode* Node, g_msg_queue_para* g_msg_queue, 
									   g_RegDev_para* g_RegDev, ThreadPool* g_threadpool);
















#endif//BS_UTILITY_H
