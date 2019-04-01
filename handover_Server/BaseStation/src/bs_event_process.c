#include "bs_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "bs_network_json.h"
#include "gw_frame.h"
#include "timer.h"

//g_network->node->system_info->

void process_network_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, zlog_category_t* zlog_handler)
{
	switch(getData->msg_type){
		case MSG_START_MONITOR: // INIT_LOCATION
		{
			zlog_info(zlog_handler,"EVENT : MSG_START_MONITOR: msg_type = %d , msg_number = %d", 
                                    getData->msg_type , getData->msg_number);
			g_network->node->system_info->bs_state = STATE_WAIT_INIT;
			zlog_info(zlog_handler,"SYSTEM STATE CHANGE : bs state STATE_STARTUP -> STATE_WAIT_INIT");

			startProcessAir(g_air,BEACON); // simulate receive beacon

			break;
		}
		case MSG_INIT_SELECTED:
		{
			zlog_info(zlog_handler,"EVENT : MSG_INIT_SELECTED: msg_type = %d , msg_number = %d", 
                                    getData->msg_type , getData->msg_number);
			/*
				1. open dac
				2. air_interface send Association request (Next_dest_mac_addr set itself)
			*/
			send_airSignal(ASSOCIATION_REQUEST, NULL, NULL, NULL, g_air);
			startProcessAir(g_air,ASSOCIATION_RESPONSE);
			break;
		}
		case MSG_START_HANDOVER:
		{
			/*
				1. air_interface send Handover start request (Next_dest_mac_addr set target)
				2. start timer thread , timer thread post timeout msg event : 
						a. check response ; b. downlink data service is over(close ddr interface)
			*/
			zlog_info(zlog_handler,"EVENT : MSG_START_HANDOVER: msg_type = %d , msg_number = %d", 
                                    getData->msg_type , getData->msg_number);
			zlog_info(zlog_handler,"EVENT : MSG_START_HANDOVER: msg_json = %s", getData->msg_json);

			send_airSignal(HANDOVER_START_REQUEST, NULL, NULL, NULL, g_air);
			startProcessAir(g_air,HANDOVER_START_RESPONSE);						
			break;
		}
		default:
			break;
	}	
}

void process_air_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, zlog_category_t* zlog_handler)
{
	switch(getData->msg_type){
		case MSG_RECEIVED_BEACON:
		{
			if(g_network->node->system_info->have_ve_mac == 1){
				zlog_info(zlog_handler,"EVENT : MSG_RECEIVED_BEACON:  break directly");
				break;
			}
			zlog_info(zlog_handler,"EVENT : MSG_RECEIVED_BEACON: msg_type = %d , msg_number = %d", getData->msg_type , getData->msg_number);
			memcpy(g_network->node->system_info->ve_mac,getData->msg_json, 6);
			g_network->node->system_info->have_ve_mac = 1;
			hexdump(g_network->node->system_info->ve_mac, 6);
			startMonitor(g_monitor,1);
			break;
		}
		case MSG_RECEIVED_ASSOCIATION_RESPONSE:
		{
			zlog_info(zlog_handler,"EVENT : MSG_RECEIVED_ASSOCIATION_RESPONSE: msg_type = %d , msg_number = %d", 
			getData->msg_type , getData->msg_number);

			// need check system state
			if(g_network->node->system_info->bs_state == STATE_WAIT_INIT){
				send_initcompleted_signal(g_network->node->my_id, g_network);
				g_network->node->system_info->bs_state == STATE_WORKING;
				zlog_info(zlog_handler,"SYSTEM STATE CHANGE : bs state STATE_WAIT_INIT -> STATE_WORKING");
				startMonitor(g_monitor,2);
			}else if(g_network->node->system_info->bs_state == STATE_WORKING){
				
			}
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_RESPONSE:
		{
			zlog_info(zlog_handler,"EVENT : MSG_RECEIVED_HANDOVER_START_RESPONSE: msg_type = %d , msg_number = %d",
				getData->msg_type , getData->msg_number);

			startProcessAir(g_air,BEACON); // tmp test
			break;
		}
		case MSG_RECEIVED_REASSOCIATION:
		{
			break;
		}
		default:
			break;
	}
}


void eventLoop(g_network_para* g_network, g_monitor_para* g_monitor, g_air_para* g_air, 
    g_msg_queue_para* g_msg_queue, zlog_category_t* zlog_handler)
{
	while(1){
		//zlog_info(zlog_handler,"wait getdata ----- \n");
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;

		if(getData->msg_type < MSG_START_MONITOR)
			process_air_event(getData, g_network, g_monitor, g_air, zlog_handler);
		else if(getData->msg_type <= MSG_TIMEOUT)
			process_network_event(getData, g_network, g_monitor, g_air, zlog_handler);
	}
}






