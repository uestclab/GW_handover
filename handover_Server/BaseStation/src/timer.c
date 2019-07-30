#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>

#include "timer.h"


void process_once(g_timer_para* g_timer){
	g_timer->running = 0;
	//usleep()
	g_timer->timer_cb(g_timer->in_data,g_timer->g_msg_queue);
}

void* timer_thread(void* args){
	g_timer_para* g_timer = (g_timer_para*)args;

	pthread_mutex_lock(g_timer->para_t->mutex_);
    while(1){
		while (g_timer->running == 0 )
		{
			zlog_info(g_timer->log_handler,"timer_thread() : wait for condition\n");
			pthread_cond_wait(g_timer->para_t->cond_, g_timer->para_t->mutex_);
		}
		pthread_mutex_unlock(g_timer->para_t->mutex_);
    	process_once(g_timer);		
    }
    zlog_info(g_timer->log_handler,"Exit timer_thread()");

}

void StartTimer(timercb_func timer_cb, g_timer_para* g_timer){
	zlog_info(g_timer->log_handler,"StartTimer()");
	g_timer->timer_cb = timer_cb;
	g_timer->running  = 1;
	pthread_cond_signal(g_timer->para_t->cond_);
}


int InitTimerThread(g_timer_para** g_timer, g_msg_queue_para* g_msg_queue, zlog_category_t* handler){
	zlog_info(handler,"InitTimerThread()");
	*g_timer = (g_timer_para*)malloc(sizeof(struct g_timer_para));
	(*g_timer)-> timer_cb = NULL;
	(*g_timer)-> running  = 0;
	(*g_timer)-> in_data = NULL; 
	(*g_timer)-> para_t = newThreadPara();
	(*g_timer)->g_msg_queue = g_msg_queue;
	(*g_timer)-> log_handler = handler;
	int ret = pthread_create((*g_timer)->para_t->thread_pid, NULL, timer_thread, (void*)(*g_timer));
    if(ret != 0){
        zlog_error(handler,"create timer_thread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}


void closeTimer(g_timer_para* g_timer){
	//g_timer-> running  = 2;
	//pthread_cond_signal(g_timer->para_t->cond_);
	//pthread_join(*(g_timer->para_t->thread_pid),NULL);
	zlog_info(g_timer->log_handler,"closeTimer()");
	// continue to release timer resource --- 
}

// ---------------------------------------------------------------------




