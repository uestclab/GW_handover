#ifndef GW_TIMER_H
#define GW_TIMER_H


#include "zlog.h"
#include "define_common.h"
#include "gw_utility.h"

#include "msg_queue.h"

typedef void (*timercb_func)(void* in_data, g_msg_queue_para* g_msg_queue); 

typedef struct g_timer_para{
	g_msg_queue_para*  g_msg_queue;
	timercb_func       timer_cb;
	void*              in_data;
	int                running;
	para_thread*       para_t;
	zlog_category_t*   log_handler;
}g_timer_para;


int InitTimerThread(g_timer_para** g_timer, g_msg_queue_para* g_msg_queue, zlog_category_t* handler);
void closeTimer(g_timer_para* g_timer);
void StartTimer(timercb_func timer_cb, g_timer_para* g_timer);


#endif//GW_TIMER_H


