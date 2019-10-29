#include <pthread.h>
#include "cJSON.h"
#include "ve_periodic.h"
#include "gw_frame.h"

int getRunningState(g_periodic_para* g_periodic, int value){
	pthread_mutex_lock(g_periodic->para_check_t->mutex_);
	if(value == 1){
		g_periodic->running = -1;
		pthread_mutex_unlock(g_periodic->para_check_t->mutex_); // note that: pthread_mutex_unlock before return !!!!! 
		return 0;
	}
	int ret = g_periodic->running;
	pthread_mutex_unlock(g_periodic->para_check_t->mutex_);
	return ret;
}

void send_air_frame(g_periodic_para* g_periodic){
	system_info_para* g_system_info = g_periodic->node->system_info;
	char my_inital[6];
	memset(my_inital,0x0,6);
	memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));

	{
		if(g_periodic->running == BEACON){ // send BEACON
			zlog_info(g_periodic->log_handler,"send BEACON periodic \n");
			for(;;){
				send_airSignal(BEACON, g_system_info->ve_mac, g_system_info->link_bs_mac, my_inital, g_periodic->g_air);	
				gw_sleep();
				int ret = getRunningState(g_periodic, 0);
				if(ret == -1)
					break;
			}
		}else if(g_periodic->running == REASSOCIATION){ // send REASSOCIATION
			int send_reassociation_cnt = 0;
			for(;;){ // next_bs_mac
				send_airSignal(REASSOCIATION, g_system_info->ve_mac, g_system_info->next_bs_mac, my_inital, g_periodic->g_air);
				//gw_sleep();
				usleep(2000);
				send_reassociation_cnt ++;
				if(send_reassociation_cnt > 10){
					printf("reassociation is send too much more ------- error !\n");
					send_reassociation_cnt = 0;
					//stopPeriodic(g_periodic);
					//break;
				}
				
				int ret = getRunningState(g_periodic, 0);
				if(ret == -1)
					break;
			}
		}
		g_periodic->running = -1;
	}
}

void* periodic_thread(void* args){
	g_periodic_para* g_periodic = (g_periodic_para*)args;

	pthread_mutex_lock(g_periodic->para_t->mutex_);
    while(1){
		while (g_periodic->running == -1 )
		{
			zlog_info(g_periodic->log_handler,"periodic_thread() : wait for start\n");
			pthread_cond_wait(g_periodic->para_t->cond_, g_periodic->para_t->mutex_);
		}
		pthread_mutex_unlock(g_periodic->para_t->mutex_);
    	send_air_frame(g_periodic);
    }
    zlog_info(g_periodic->log_handler,"Exit periodic_thread()");

}

void startPeriodic(g_periodic_para* g_periodic, int running_step){
	g_periodic->running = running_step;
	pthread_cond_signal(g_periodic->para_t->cond_);
}

void stopPeriodic(g_periodic_para* g_periodic){
	int ret = getRunningState(g_periodic, 1);
	zlog_info(g_periodic->log_handler,"stopPeriodic()");
}

int initPeriodicThread(struct ConfigureNode* Node, g_periodic_para** g_periodic, g_air_para* g_air,
		g_msg_queue_para* g_msg_queue, zlog_category_t* handler)
{
	zlog_info(handler,"initPeriodicThread()");
	*g_periodic = (g_periodic_para*)malloc(sizeof(struct g_periodic_para));
	(*g_periodic)->log_handler = handler;
	(*g_periodic)->para_t = newThreadPara();
	(*g_periodic)->para_check_t = newThreadPara();
	(*g_periodic)->running = -1;
	(*g_periodic)->g_msg_queue = g_msg_queue;
	(*g_periodic)->node = Node;
	(*g_periodic)->g_air = g_air;

	//zlog_info(handler,"g_msg_queue->msgid = %d \n" , (*g_periodic)->g_msg_queue->msgid);
	int ret = pthread_create((*g_periodic)->para_t->thread_pid, NULL, periodic_thread, (void*)(*g_periodic));
    if(ret != 0){
        zlog_error(handler,"create initPeriodicThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

int freePeriodicThread(g_periodic_para* g_periodic){
	zlog_info(g_periodic->log_handler,"freePeriodicThread()");
	free(g_periodic);
}





