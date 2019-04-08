#include "bs_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "bs_network_json.h"
#include "gw_frame.h"
#include "gw_control.h"

#include "timer.h"

int is_my_air_frame(char* src, char* dest){
	int i = 0;
	for(i=0;i<6;i++){
		if(src[i] != dest[i])
			return -1;
	}
	return 0;
}

char* msgJsonSourceMac(char* msg_json){
	return msg_json;
}

char* msgJsonDestMac(char* msg_json){
	return msg_json + 6;
}

char* msgJsonNextDstMac(char* msg_json){
	return msg_json + 12;
}

void configureDstMacToBB(char* link_ve_mac, zlog_category_t* zlog_handler){
	char* high16str = getHigh16Str(link_ve_mac);
	zlog_info(zlog_handler,"high16str = %s\n",high16str);
	char* low32str = getLow32Str(link_ve_mac);
	zlog_info(zlog_handler,"low32str = %s\n",low32str);
	int ret = set_dst_mac(low32str, high16str);
	zlog_info(zlog_handler,"set_dst_mac : ret = %d \n", ret);
	free(high16str);
	free(low32str);
}

void process_network_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_START_MONITOR: // INIT_LOCATION
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_MONITOR: msg_number = %d",getData->msg_number);
			g_system_info->bs_state = STATE_WAIT_MONITOR;
			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_STARTUP -> STATE_WAIT_MONITOR");

			startProcessAir(g_air,1);			

			//startProcessAir(g_air,BEACON); // simulate receive beacon

			break;
		}
		case MSG_INIT_SELECTED:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_INIT_SELECTED: msg_number = %d",getData->msg_number);
			/* open dac */
			//enable_dac();
			/* air_interface send Association request (Next_dest_mac_addr set itself) */
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			//startProcessAir(g_air,ASSOCIATION_RESPONSE);
			break;
		}
		case MSG_START_HANDOVER: // !!!!!!!!!!!!!!!!!!!!!!!! source bs start to disconnect ve
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_HANDOVER: msg_number = %d",getData->msg_number);
			
			hexdump(msgJsonSourceMac(getData->msg_json),6);
			/* 1. air_interface send Handover start request (Next_dest_mac_addr set target) */
			send_airSignal(HANDOVER_START_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, msgJsonSourceMac(getData->msg_json), g_air);
			/* 2. start timer (timeout msg event): a. check response ; b. downlink data service is over(close ddr interface) */
			// start_timer();
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
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_RECEIVED_BEACON:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_BEACON: msg_number = %d",getData->msg_number);
			if(g_system_info->have_ve_mac == 0){
				memcpy(g_system_info->ve_mac,msgJsonSourceMac(getData->msg_json), 6);
				g_system_info->have_ve_mac = 1;
				configureDstMacToBB(g_system_info->ve_mac,zlog_handler);
				zlog_info(zlog_handler," BEACON receive ve_mac = : ");
				hexdump(g_system_info->ve_mac, 6);
			}

			if(g_system_info->bs_state != STATE_WAIT_MONITOR){
				zlog_info(zlog_handler," ----- Not in state of STATE_WAIT_MONITOR ------ ");
				break;
			}

			startMonitor(g_monitor,1); // simulate 
			break;
		}
		case MSG_RECEIVED_ASSOCIATION_RESPONSE:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_ASSOCIATION_RESPONSE: msg_number = %d",getData->msg_number);

			/* check dest mac */
			if(is_my_air_frame(g_system_info->bs_mac, msgJsonDestMac(getData->msg_json)) != 0){
				zlog_info(zlog_handler," check dest MAC fail , not to my air frame");
				break;
			}

			// need check system state
			if(g_system_info->bs_state == STATE_WAIT_MONITOR){
				send_initcompleted_signal(g_network->node->my_id, g_network);
				g_system_info->bs_state = STATE_WORKING;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_WORKING");
				//startMonitor(g_monitor,2);
			}else if(g_system_info->bs_state == STATE_WORKING){
				
			}
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_RESPONSE:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_HANDOVER_START_RESPONSE: msg_number = %d",getData->msg_number);

			/* check dest mac */
			if(is_my_air_frame(g_system_info->bs_mac, msgJsonDestMac(getData->msg_json)) != 0){
				break;
			}
	
			if(g_system_info->received_start_handover_response == 0){
				g_system_info->received_start_handover_response = 1;
			}		

			break;
		}
		case MSG_RECEIVED_REASSOCIATION:// !!!!!!!!!!!!!!!!!!!!!!!! ve start to connect target bs
		{

			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_REASSOCIATION: msg_number = %d",getData->msg_number);

			if(g_system_info->have_ve_mac == 0){
				memcpy(g_system_info->ve_mac,msgJsonSourceMac(getData->msg_json), 6);
				g_system_info->have_ve_mac = 1;
				configureDstMacToBB(g_system_info->ve_mac,zlog_handler);
				zlog_info(zlog_handler," REASSOCIATION receive ve_mac = : ");
				hexdump(g_system_info->ve_mac, 6);
			}

			/* check dest mac and state */
			if(g_system_info->bs_state == STATE_WAIT_MONITOR){ // for target bs and other waiting bs
				if(is_my_air_frame(g_system_info->bs_mac, msgJsonDestMac(getData->msg_json)) != 0)// waiting bs
					break;
				/* open dac */
				//enable_dac();
				send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
				/* air_mac_id synchronous */
				// ----------- continue 0401
				
			}else if(g_system_info->bs_state == STATE_WORKING){ // for source bs
				/* close dac */
				//disable_dac();
				send_linkclosed_signal(g_network->node->my_id, g_network);
				/* close ddr_interface */
				//close_ddr_interface();
				g_system_info->bs_state == STATE_WAIT_MONITOR;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WORKING -> STATE_WAIT_MONITOR");
			}

			break;
		}
		default:
			break;
	}
}

void process_self_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_TIMEOUT:
		{
			/* 1. close ddr interface , downlink service is close */
			// close_ddr_interface();
			/* 2. check if received air_response */
			if(g_system_info->received_start_handover_response == 0){
				
			}else{
				
			}
			/* 3. send DEASSOCIATION via air */
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
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
		else if(getData->msg_type < MSG_TIMEOUT)
			process_network_event(getData, g_network, g_monitor, g_air, zlog_handler);
		else if(getData->msg_type >= MSG_TIMEOUT)
			process_self_event(getData, g_network, g_monitor, g_air, zlog_handler);

		free(getData);
	}
}






