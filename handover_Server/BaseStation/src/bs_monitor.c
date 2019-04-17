#include <pthread.h>
#include "bs_monitor.h"
#include "bs_network_json.h"
#include "cJSON.h"

void monitor_loop(g_monitor_para* g_monitor){

	int quility = 0;
	int correct_d = 0;
	int error_d = 0;

	// get init compare value

	uint32_t pre_correct_cnt = get_crc_correct_cnt(g_monitor->g_RegDev);
	uint32_t pre_error_cnt = get_crc_error_cnt(g_monitor->g_RegDev);
	
	while(1){
		usleep(1000);
		// monitor crc and power latch
		if(getPowerLatch(g_monitor->g_RegDev) < 400000){
			zlog_info(g_monitor->log_handler,"PowerLatch < 400000 \n");
			continue;
		}
		correct_d = get_crc_correct_cnt(g_monitor->g_RegDev) - pre_correct_cnt;
		pre_correct_cnt = get_crc_correct_cnt(g_monitor->g_RegDev);

		error_d = get_crc_error_cnt(g_monitor->g_RegDev) - pre_error_cnt;
		pre_error_cnt = get_crc_error_cnt(g_monitor->g_RegDev);
		
		quility = correct_d - error_d + quility;
		if(quility > 15)
			break;			
	}

	send_ready_handover_signal(g_monitor->node->my_id, g_monitor->node->my_mac_str, quility, g_monitor->g_network);
}

void monitor(g_monitor_para* g_monitor){
	
	{
		if(g_monitor->running == 1){ // simulate RELOCALIZATION STATE
			zlog_info(g_monitor->log_handler,"simulate ready_handover in RELOCALIZATION STATE\n");
			send_ready_handover_signal(g_monitor->node->my_id, g_monitor->node->my_mac_str, 10, g_monitor->g_network);
		}else if(g_monitor->running == 2){ // simulate RUNNING STATE
			zlog_info(g_monitor->log_handler,"simulate ready_handover in RUNNING STATE\n");
			send_ready_handover_signal(g_monitor->node->my_id, g_monitor->node->my_mac_str, 20, g_monitor->g_network);
		}else if(g_monitor->running == 3){
			zlog_info(g_monitor->log_handler,"start monitor process to check if trigger ready_handover \n");
			monitor_loop(g_monitor); 
		}
		g_monitor->running = 0;
	}

}

void* monitor_thread(void* args){
	g_monitor_para* g_monitor = (g_monitor_para*)args;

	pthread_mutex_lock(g_monitor->para_t->mutex_);
    while(1){
		while (g_monitor->running == 0 )
		{
			zlog_info(g_monitor->log_handler,"monitor_thread() : wait for start\n");
			pthread_cond_wait(g_monitor->para_t->cond_, g_monitor->para_t->mutex_);
		}
		pthread_mutex_unlock(g_monitor->para_t->mutex_);
    	monitor(g_monitor);
		//if(g_monitor->running == 0)
		//	break;
    }
    zlog_info(g_monitor->log_handler,"Exit monitor_thread()");

}

void startMonitor(g_monitor_para* g_monitor, int running_step){
	g_monitor->running = running_step;
	pthread_cond_signal(g_monitor->para_t->cond_);
}

int initMonitorThread(struct ConfigureNode* Node, g_monitor_para** g_monitor, 
		g_msg_queue_para* g_msg_queue, g_network_para* g_network, g_RegDev_para* g_RegDev, zlog_category_t* handler)
{
	zlog_info(handler,"initMonitorThread()");
	*g_monitor = (g_monitor_para*)malloc(sizeof(struct g_monitor_para));
	(*g_monitor)->log_handler = handler;
	(*g_monitor)->para_t = newThreadPara();
	(*g_monitor)->running = 0;
	(*g_monitor)->g_msg_queue = g_msg_queue;
	(*g_monitor)->g_network = g_network;
	(*g_monitor)->g_RegDev = g_RegDev;
	(*g_monitor)->node = Node;

	//zlog_info(handler,"g_msg_queue->msgid = %d \n" , (*g_monitor)->g_msg_queue->msgid);
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





