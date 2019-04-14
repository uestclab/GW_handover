#include "bs_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "bs_network_json.h"
#include "gw_frame.h"
#include "gw_control.h"

#include "timer.h"

void simulate_single_trigger_handover(g_network_para* g_network, g_monitor_para* g_monitor){

	for(int i=0; i<10;i++){
		gw_sleep();
	}

	system_info_para* g_system_info = g_network->node->system_info;

	startMonitor(g_monitor,2); // ------- notify bs handover : simulate code , to trigger start handover
	g_system_info->bs_state = STATE_WAIT_MONITOR; // temp change state to simulate source bs and target bs on 1 board currently;
	
	zlog_info(g_network->log_handler," ------ simulate source bs turn to target bs >>>>>>>>>>>>>>>>>>>>>>>>> -------------\n");
}


void timer_cb(void* in_data, g_msg_queue_para* g_msg_queue){
	struct msg_st data;
	data.msg_type = MSG_TIMEOUT;
	data.msg_number = MSG_TIMEOUT;
	data.msg_len = 0;
	postMsgQueue(&data,g_msg_queue);
}


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

void configureDstMacToBB(char* dst_buf, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	zlog_info(zlog_handler," start set_dst_mac_fast \n");
	set_dst_mac_fast(g_RegDev, dst_buf);
	zlog_info(zlog_handler," end set_dst_mac_fast \n");
}

void process_network_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_START_MONITOR: // INIT_LOCATION
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_MONITOR: msg_number = %d",getData->msg_number);
			g_system_info->bs_state = STATE_WAIT_MONITOR;
			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_STARTUP -> STATE_WAIT_MONITOR");

			startProcessAir(g_air,1); // open air signal receive

			break;
		}
		case MSG_INIT_SELECTED:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_INIT_SELECTED: msg_number = %d",getData->msg_number);

			enable_dac(g_RegDev);  // open dac and check is enable successful --- continue

			// air_interface send Association request (Next_dest_mac_addr set itself) -- first air frame sent by bs ---!
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);

			g_system_info->bs_state = STATE_INIT_SELECTED;
			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_INIT_SELECTED");

			break;
		}
		case MSG_START_HANDOVER: // !!!!!!!!!!!!!!!!!!!!!!!! source bs start to disconnect ve  , A1 event --> A2 event
		{
			zlog_info(zlog_handler," ---------------------- A1 Events start --------------------------");
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_HANDOVER: msg_number = %d",getData->msg_number);
			printf("target bs mac : \n");
			hexdump(msgJsonSourceMac(getData->msg_json),6);

			/* 1. air_interface send Handover start request (Next_dest_mac_addr set target) */
			send_airSignal(HANDOVER_START_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, msgJsonSourceMac(getData->msg_json), g_air);
			/* 2. start timer (timeout msg event): a. check response ; b. downlink data service is over(close ddr interface) */
			StartTimer(timer_cb, NULL, 0, 1000, g_air->g_timer);
				
			break;
		}
		default:
			break;
	}	
}

void process_air_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_RECEIVED_BEACON:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_BEACON: msg_number = %d",getData->msg_number);
			if(g_system_info->have_ve_mac == 0){
				memcpy(g_system_info->ve_mac,msgJsonSourceMac(getData->msg_json), 6);
				g_system_info->have_ve_mac = 1;
				configureDstMacToBB(g_system_info->ve_mac,g_RegDev,zlog_handler);
				printf(" BEACON receive ve_mac = : \n");
				hexdump(g_system_info->ve_mac, 6);
			}

			if(g_system_info->bs_state != STATE_WAIT_MONITOR){
				zlog_info(zlog_handler," ----- Not in state of STATE_WAIT_MONITOR ------ ");
				break;
			}

			startMonitor(g_monitor,1); // ------- notify bs start to monitor : simulate code -------------------- first trigger ready_handover
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
			if(g_system_info->bs_state == STATE_INIT_SELECTED){
				send_initcompleted_signal(g_network->node->my_id, g_network);
				open_ddr(g_RegDev); 
				g_system_info->bs_state = STATE_WORKING;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_INIT_SELECTED -> STATE_WORKING");
				zlog_info(zlog_handler," ---------------------- B1 Events completed --------------------------\n");
		

// -------------- test point 1 : end test INIT relocation 0411 ----------------------------------------------------------------------------------

				printf(" ----!!!!!!! current bs start to trigger ready_handover , wait gw_sleep() 10s ----testpoint_1---------\n");
				
				simulate_single_trigger_handover(g_network, g_monitor); // simulate trigger ready_handover
				
// ------------------------------------------------------------------------------------------------------------------------------------------
			}else if(g_system_info->bs_state == STATE_TARGET_SELECTED){

				open_ddr(g_RegDev);
				trigger_mac_id(g_RegDev);

				g_system_info->bs_state = STATE_WORKING;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_TARGET_SELECTED -> STATE_WORKING");

				printf(" ----!!!!!!! current bs start to trigger ready_handover , wait gw_sleep() 10s ----testpoint_2---------\n");

				simulate_single_trigger_handover(g_network, g_monitor);
			}
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_RESPONSE:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_HANDOVER_START_RESPONSE: msg_number = %d",getData->msg_number);

			/* check dest mac */
			if(is_my_air_frame(g_system_info->bs_mac, msgJsonDestMac(getData->msg_json)) != 0){
				zlog_info(zlog_handler," check dest MAC fail , not to my air frame");
				break;
			}
	
			if(g_system_info->received_start_handover_response == 0){
				g_system_info->received_start_handover_response = 1;
			}

			/* 3. send DEASSOCIATION via air */
			close_ddr(g_RegDev);
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			
			/* add read regsiter to ensure DEASSOCIATION transmit successful before disable_dac */
		
			disable_dac(g_RegDev); // check ve received DEASSOCIATION ????????????????? --- 0414

			zlog_info(zlog_handler," ----------------test point 2: End A2 event \n");


			break;
		}
		case MSG_RECEIVED_REASSOCIATION:// !!!!!!!!!!!!!!!!!!!!!!!! ve start to connect target bs
		{

			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_REASSOCIATION: msg_number = %d",getData->msg_number);

			if(g_system_info->have_ve_mac == 0){
				memcpy(g_system_info->ve_mac,msgJsonSourceMac(getData->msg_json), 6);
				g_system_info->have_ve_mac = 1;
				configureDstMacToBB(g_system_info->ve_mac,g_RegDev,zlog_handler);
				zlog_info(zlog_handler," REASSOCIATION receive ve_mac = : ");
				hexdump(g_system_info->ve_mac, 6);
			}

			/* check dest mac and state */
			if(g_system_info->bs_state == STATE_WAIT_MONITOR){ // for target bs and other waiting bs
				if(is_my_air_frame(g_system_info->bs_mac, msgJsonDestMac(getData->msg_json)) != 0)// waiting bs
					break;

				// for target bs
				enable_dac(g_RegDev);
				send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);

				g_system_info->bs_state = STATE_TARGET_SELECTED;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_TARGET_SELECTED");

				
			}else if(g_system_info->bs_state == STATE_WORKING){ // for source bs
				send_linkclosed_signal(g_network->node->my_id, g_network);
				g_system_info->bs_state = STATE_WAIT_MONITOR;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WORKING -> STATE_WAIT_MONITOR");
			}

			break;
		}
		default:
			break;
	}
}

void process_self_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_TIMEOUT:
		{

			zlog_info(zlog_handler," ---------------- EVENT : MSG_TIMEOUT: msg_number = %d",getData->msg_number);
			/* 1. close ddr interface , downlink service is close */
			/* 2. check if received air_response */
			if(g_system_info->received_start_handover_response == 0){
				zlog_info(zlog_handler," ---- Not receive start handover response in expect time !!!! ");
			}else{
				
			}

// -------------- test point 2: End A2 event  ---------------------------------------------------------------------------------

			break;
		}
		default:
			break;
	}
}

void init_state(g_network_para* g_network, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	zlog_info(zlog_handler," init_state() ----- \n");
	system_info_para* g_system_info = g_network->node->system_info;
	
	/* init src_mac */
	zlog_info(zlog_handler," start set_src_mac_fast \n");
	int ret = set_src_mac_fast(g_RegDev, g_system_info->bs_mac);
	zlog_info(zlog_handler,"end set_src_mac_fast : ret = %d \n", ret);
	zlog_info(zlog_handler,"completed set_src_mac_fast ---------- \n");

	disable_dac(g_RegDev);
	close_ddr(g_RegDev);
}

void eventLoop(g_network_para* g_network, g_monitor_para* g_monitor, g_air_para* g_air, 
    g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
{

	init_state(g_network, g_RegDev,zlog_handler);

	zlog_info(zlog_handler," ------------------------------  start baseStation event loop ----------------------------- \n");

	while(1){
		//zlog_info(zlog_handler,"wait getdata ----- \n");
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;

		if(getData->msg_type < MSG_START_MONITOR)
			process_air_event(getData, g_network, g_monitor, g_air, g_RegDev, zlog_handler);
		else if(getData->msg_type < MSG_TIMEOUT)
			process_network_event(getData, g_network, g_monitor, g_air, g_RegDev, zlog_handler);
		else if(getData->msg_type >= MSG_TIMEOUT)
			process_self_event(getData, g_network, g_monitor, g_air, g_RegDev, zlog_handler);

		free(getData);
	}
}






