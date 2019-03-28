#include "bs_monitor.h"
#include "gw_utility.h"


void monitor(g_monitor_para* g_monitor){
	zlog_info(g_monitor->log_handler,"Enter monitor()");
	user_wait();
	zlog_info(g_monitor->log_handler,"exit monitor()");
}

void* monitor_thread(void* args){
	g_monitor_para* g_monitor = (g_monitor_para*)args;

	pthread_mutex_lock(g_monitor->para_t->mutex_);
    while(1){
		while (g_monitor->running == 0 )
		{
			zlog_info(g_monitor->log_handler,"monitor_thread() : wait for condition\n");
			pthread_cond_wait(g_monitor->para_t->cond_, g_monitor->para_t->mutex_);
		}
		pthread_mutex_unlock(g_monitor->para_t->mutex_);
    	monitor(g_monitor);
    }
    zlog_info(g_monitor->log_handler,"Exit monitor_thread()");

}

void startMonitor(g_monitor_para* g_monitor){
	g_monitor->running = 1;

	pthread_cond_signal(g_monitor->para_t->cond_);
}

int initMonitorThread(struct ConfigureNode* Node, g_monitor_para** g_monitor, zlog_category_t* handler)
{
	zlog_info(handler,"initMonitorThread()");
	*g_monitor = (g_monitor_para*)malloc(sizeof(struct g_monitor_para));
	(*g_monitor)->log_handler = handler;
	(*g_monitor)->para_t = newThreadPara();
	(*g_monitor)->running = 0;
	
	zlog_info(handler,"g_monitor = %d , para_t = %d , running = %d \n" , *g_monitor , (*g_monitor)->para_t, (*g_monitor)->running);
	int ret = pthread_create((*g_monitor)->para_t->thread_pid, NULL, monitor_thread, (void*)(*g_monitor));
    if(ret != 0){
        zlog_error(handler,"create monitor_thread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

int freeMonitorThread(zlog_category_t* handler){
	zlog_info(handler,"freeMonitorThread()");
}
