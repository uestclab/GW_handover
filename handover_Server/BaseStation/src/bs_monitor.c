#include <pthread.h>
#include "bs_monitor.h"
#include "bs_network_json.h"
#include "bs_utility.h"
#include "cJSON.h"

/* ----------------------------------- use thread pool ---------------------------------------------------- */
void postSendReadyHandoverSignal(int32_t quility, g_msg_queue_para* g_msg_queue){
	postMsgWrapper(MSG_MONITOR_READY_HANDOVER, (char*)(&quility), sizeof(int32_t), g_msg_queue); // 0319 note --- _lq_
}

/* 
	1. check rx mcs, snr, crc
	2. add punish to avoid handover to source bs immediately 
*/

void* wait_bs_monitor_loop(void* args){
	g_monitor_para* g_monitor = (g_monitor_para*)args;

	int32_t quility = 0;

	for(int i = 0 ; i < g_monitor->node->delay_mon_cnt_second ; i++)
		gw_sleep();

	// get init compare value

	double snr = get_rx_snr(g_monitor->g_RegDev);
	
	while(1){
		usleep(10000);
		// monitor crc and power latch
		if(getPowerLatch(g_monitor->g_RegDev) < 150000){
			//zlog_info(g_monitor->log_handler,"PowerLatch < 150000 \n");
			continue;
		}
	
		snr = get_rx_snr(g_monitor->g_RegDev);

		if(snr > g_monitor->node->snr_threshold){
			quility = (int32_t)snr;
			//printf("waiting bs snr : %f , ready for handover\n",snr);
			postSendReadyHandoverSignal(quility, g_monitor->g_msg_queue);
			break;
		}		
	}
	free(g_monitor);
}

// working source bs monitor itself with distance  
void* source_bs_monitor_loop(void* args){
	g_monitor_para* g_monitor = (g_monitor_para*)args;

	system_info_para* g_system_info = g_monitor->node->system_info;

	usleep(10000);

	uint32_t delay =(g_system_info->my_initial+g_system_info->other_initial)/2+32;

	uint32_t current_tick = get_delay_tick(g_monitor->g_RegDev);

	double distance = (current_tick - delay) * 0.149896229;

	double snr = get_rx_snr(g_monitor->g_RegDev);

	// 0 : bpsk , 1 : qpsk 2 , 2 : 16qam
	uint32_t rx_mcs = get_rx_mcs(g_monitor->g_RegDev);
	
	while(g_system_info->bs_state == STATE_WORKING){
		usleep(500000); // 0.5s

		distance = (get_delay_tick(g_monitor->g_RegDev) - delay) * 0.149896229;
		snr = get_rx_snr(g_monitor->g_RegDev);

		//zlog_info(g_monitor->g_network->log_handler,"source bs monitor itself : distance = %f, snr = %f \n", distance,snr);

		if(distance < g_monitor->node->distance_threshold){
			postMsgWrapper(MSG_SOURCE_BS_DISTANCE_NEAR, NULL, 0, g_monitor->g_network->g_msg_queue);
			break;
		}	
	}
	free(g_monitor);
}

void postMonitorWorkToThreadPool(struct ConfigureNode* Node, g_msg_queue_para* g_msg_queue, g_network_para* g_network, 
								 g_RegDev_para* g_RegDev, ThreadPool* g_threadpool, int running_step)
{
	struct g_monitor_para* g_monitor = (g_monitor_para*)malloc(sizeof(g_monitor_para));
	g_monitor->g_msg_queue = g_msg_queue;
	g_monitor->g_RegDev = g_RegDev;
	g_monitor->g_network = g_network;
	g_monitor->node = Node;

	if (running_step == 3)
		AddWorker(wait_bs_monitor_loop,(void*)g_monitor,g_threadpool);
	else if (running_step == 4)
		AddWorker(source_bs_monitor_loop,(void*)g_monitor,g_threadpool);
		
}











