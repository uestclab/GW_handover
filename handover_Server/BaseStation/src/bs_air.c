#include "bs_air.h"



void process_air(g_air_para* g_air){
	zlog_info(g_air->log_handler,"Enter process_air()");
	
	{
			struct msg_st data;
			data.msg_type = MSG_AIR;
			data.json[0] = 'A';
			data.json[1] = '\0';
			data.msg_number = 0;
	
			int counter = 1000;
			while(counter > 0){
				enqueue(&data,g_air->g_msg_queue);
				data.msg_number = data.msg_number + 1;
				counter = counter - 1;
			}
			g_air->running = 0;
	}
	
	zlog_info(g_air->log_handler,"exit process_air()");
}

void* process_air_thread(void* args){
	g_air_para* g_air = (g_air_para*)args;

	pthread_mutex_lock(g_air->para_t->mutex_);
    while(1){
		while (g_air->running == 0 )
		{
			zlog_info(g_air->log_handler,"process_air_thread() : wait for condition\n");
			pthread_cond_wait(g_air->para_t->cond_, g_air->para_t->mutex_);
		}
		pthread_mutex_unlock(g_air->para_t->mutex_);
    	process_air(g_air);
		if(g_air->running == 0)
			break;
    }
    zlog_info(g_air->log_handler,"Exit process_air_thread()");

}

int initProcessAirThread(struct ConfigureNode* Node, g_air_para** g_air, g_msg_queue_para*  g_msg_queue, zlog_category_t* handler){
	zlog_info(handler,"initProcessAirThread()");
	*g_air = (g_air_para*)malloc(sizeof(struct g_air_para));
	(*g_air)->log_handler = handler;
	(*g_air)->para_t = newThreadPara();
	(*g_air)->running = 0;
	(*g_air)->g_msg_queue = g_msg_queue;

	zlog_info(handler,"g_msg_queue->msgid = %d \n" , (*g_air)->g_msg_queue->msgid);
	int ret = pthread_create((*g_air)->para_t->thread_pid, NULL, process_air_thread, (void*)(*g_air));
    if(ret != 0){
        zlog_error(handler,"create monitor_thread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

int  freeProcessAirThread(g_air_para* g_air){
	zlog_info(g_air->log_handler,"freeProcessAirThread()");
	free(g_air);
}

void startProcessAir(g_air_para* g_air){
	g_air->running = 1;
	pthread_cond_signal(g_air->para_t->cond_);
}
