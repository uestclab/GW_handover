#include "bs_monitor.h"
#include "gw_utility.h"


void monitor(g_monitor_para* g_monitor){
	zlog_info(g_monitor->log_handler,"Enter monitor()");
	
	{
			struct msg_st data;
			data.msg_type = MSG_MONITOR;
			data.json[0] = 'a';
			data.json[1] = '\0';
			data.msg_number = 0;
	
			int counter = 1000;
			while(counter > 0){
				enqueue(&data,g_monitor->g_msg_queue);
				data.msg_number = data.msg_number + 1;
				counter = counter - 1;
			}
			g_monitor->running = 0;
	}
	
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
		if(g_monitor->running == 0)
			break;
    }
    zlog_info(g_monitor->log_handler,"Exit monitor_thread()");

}

void startMonitor(g_monitor_para* g_monitor){
	g_monitor->running = 1;
	pthread_cond_signal(g_monitor->para_t->cond_);
}

int initMonitorThread(struct ConfigureNode* Node, g_monitor_para** g_monitor, g_msg_queue_para* g_msg_queue, zlog_category_t* handler)
{
	zlog_info(handler,"initMonitorThread()");
	*g_monitor = (g_monitor_para*)malloc(sizeof(struct g_monitor_para));
	(*g_monitor)->log_handler = handler;
	(*g_monitor)->para_t = newThreadPara();
	(*g_monitor)->running = 0;
	(*g_monitor)->g_msg_queue = g_msg_queue;

	zlog_info(handler,"g_msg_queue->msgid = %d \n" , (*g_monitor)->g_msg_queue->msgid);
	int ret = pthread_create((*g_monitor)->para_t->thread_pid, NULL, monitor_thread, (void*)(*g_monitor));
    if(ret != 0){
        zlog_error(handler,"create monitor_thread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

int freeMonitorThread(g_monitor_para* g_monitor){
	zlog_info(g_monitor->log_handler,"freeMonitorThread()");
	free(g_monitor);
}





