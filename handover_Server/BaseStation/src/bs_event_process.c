#include "bs_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "bs_network_json.h"
#include "gw_frame.h"
#include "gw_control.h"
#include "bs_utility.h"
#include <assert.h>

// distance measure related function
void self_test_send(g_air_para* g_air){
	system_info_para* g_system_info = g_air->node->system_info;
	 // send DISTANC_MEASURE_REQUEST
	zlog_info(g_air->log_handler,"send DISTANC_MEASURE_REQUEST self_test_send \n");
	for(int i = 0; i < 10 ; i++){
		send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
		usleep(100000);
	}	
}

void Enter_self_test(g_air_para* g_air , g_RegDev_para* g_RegDev){
	system("sh ../conf/open_self.sh");

	zlog_info(g_air->log_handler,"open_self");

	self_test_send(g_air);

	g_air->node->system_info->my_initial = get_delay_tick(g_RegDev);
	g_air->node->system_info->have_my_initial = 1;

	system("sh ../conf/close_self.sh");

	zlog_info(g_air->log_handler,"close_self, my_initial = %u", g_air->node->system_info->my_initial);
}


// -----------------------------------------------------------------------------------------------------------

/* network event */
void process_network_event(struct msg_st* getData, g_network_para* g_network, g_air_para* g_air, 
                          g_x2_para* g_x2, g_RegDev_para* g_RegDev, g_msg_queue_para* g_msg_queue, 
				          ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_START_MONITOR: // INIT_LOCATION
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_MONITOR: msg_number = %d",getData->msg_number);
			g_system_info->bs_state = STATE_WAIT_MONITOR;
			zlog_info(zlog_handler," SYSTEM STATE CHANGE : bs state STATE_STARTUP -> STATE_WAIT_MONITOR");

			startProcessAir(g_air,1); // open air signal receive

			break;
		}
		case MSG_INIT_DISTANCE:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_INIT_DISTANCE: msg_number = %d",getData->msg_number);
			char* msg_json = getData->msg_json;
			assert(getData->msg_len != 0);

			cJSON * root = NULL;
			cJSON * item = NULL;
			root = cJSON_Parse(msg_json);
			item = cJSON_GetObjectItem(root, "ve_id");
			int ve_id = item->valueint;
			cJSON_Delete(root);
			
			enable_dac(g_RegDev);
			self_test_send(g_air);

			uint32_t delay =(g_system_info->my_initial+g_system_info->other_initial)/2+32;
			uint32_t current_tick = get_delay_tick(g_RegDev);
			double distance = (current_tick - delay) * 0.149896229;
			disable_dac(g_RegDev);
			send_init_dist_over_signal(g_network->node->my_id, ve_id, distance, g_network);

			break;
		}
		case MSG_INIT_SELECTED:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_INIT_SELECTED: msg_number = %d",getData->msg_number);

			enable_dac(g_RegDev);

			// Next_dest_mac_addr set itself -- first air frame sent by bs ---!
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t)); // 20191024
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
			postCheckWorkToThreadPool(ASSOCIATION_REQUEST, g_msg_queue, g_air, g_threadpool);
			g_system_info->bs_state = STATE_INIT_SELECTED;
			zlog_info(zlog_handler," SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_INIT_SELECTED");

			break;
		}
		case MSG_START_HANDOVER: // source bs start to disconnect ve ..
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_HANDOVER: msg_number = %d",getData->msg_number);
			
			struct net_data* netMsg = parseNetMsg(getData);
			memcpy(g_system_info->next_bs_mac,netMsg->target_bs_mac,6); // temp save next bs mac
			configure_target_ip(netMsg->target_bs_ip, g_x2);
			free(netMsg);

			g_system_info->received_handover_start_confirm = 1;
			if(g_system_info->distance_near_confirm == 0){
				break;
			}

			// start handover progress .....
			postMsgWrapper(MSG_CONFIRM_START_HANDOVER, NULL, 0, g_network->g_msg_queue);
			break;
		}
		case MSG_SERVER_RECALL_MONITOR:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_SERVER_RECALL_MONITOR: msg_number = %d",getData->msg_number);
			if(g_system_info->monitored == 0 && g_system_info->bs_state == STATE_WAIT_MONITOR){
				g_system_info->monitored = 1;
				postMonitorWorkToThreadPool(g_network->node, g_msg_queue, g_network, g_RegDev, g_threadpool, 3);
			}else{
				zlog_info(zlog_handler," already in monitor or STATE_WORKING %d , %d ",g_system_info->monitored, g_system_info->bs_state);
			}
			break;
		}
		case MSG_SOURCE_BS_DAC_CLOSED:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_SOURCE_BS_DAC_CLOSED: msg_number = %d",getData->msg_number);
			if(g_system_info->received_network_state_list->received_dac_closed_x2 == 1)
				break;
			g_system_info->received_network_state_list->received_dac_closed_x2 = 1;
			g_system_info->sourceBs_dac_disabled = 1;
			if(g_system_info->received_reassociation == 0){
				break;				
			}

			// for target bs
			postMsgWrapper(MSG_TARGET_BS_START_WORKING, NULL, 0, g_x2->g_msg_queue);
			break;
		}
		case MSG_RECEIVED_DAC_CLOSED_ACK:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_DAC_CLOSED_ACK: msg_number = %d",getData->msg_number);
			g_system_info->received_network_state_list->received_dac_closed_x2_ack = 1;
			break;
		}
		default:
			break;
	}	
}

/* air event */
void process_air_event(struct msg_st* getData, g_network_para* g_network, g_air_para* g_air, 
                       g_x2_para* g_x2, g_RegDev_para* g_RegDev, g_msg_queue_para* g_msg_queue, 
				       ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	
	struct air_data* air_msg = parseAirMsg(getData->msg_json);

	if(g_system_info->rcv_id == air_msg->seq_id){
		printMsgType(getData->msg_type, air_msg->seq_id);
	}
	g_system_info->rcv_id = air_msg->seq_id;

	switch(getData->msg_type){
		case MSG_RECEIVED_BEACON:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_BEACON: msg_number = %d ", getData->msg_number);

			if(g_system_info->have_ve_mac == 0){
				memcpy(g_system_info->ve_mac,air_msg->source_mac_addr, 6);
				g_system_info->have_ve_mac = 1;
				configureDstMacToBB(g_system_info->ve_mac,g_RegDev,zlog_handler);
				printf(" BEACON receive ve_mac = : \n");
				hexdump(g_system_info->ve_mac, 6);
			}

			// beacon and reassocication carry initial value -- 20191023
			if(g_system_info->have_other_initial == 0){
				uint32_t other_inital = 0;
				memcpy((char*)(&other_inital), air_msg->Next_dest_mac_addr, sizeof(uint32_t));
				g_system_info->other_initial = other_inital;
				g_system_info->have_other_initial = 1;

				uint8_t ve_id = 0;
				memcpy((char*)(&ve_id), air_msg->Next_dest_mac_addr + sizeof(uint32_t), sizeof(uint8_t));
				g_system_info->ve_id = (int32_t)ve_id;
			}

			// debug code
			if(g_system_info->bs_state != STATE_WAIT_MONITOR){
				zlog_info(zlog_handler," ----- Not in state of STATE_WAIT_MONITOR ------ ");
				break;
			}

			send_recvbeacon_signal(g_network->node->my_id, g_system_info->ve_id, g_network);

			break;
		}
		case MSG_RECEIVED_ASSOCIATION_RESPONSE:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_ASSOCIATION_RESPONSE: msg_number = %d ", getData->msg_number);

			/* check dest mac */
			if(is_my_air_frame(g_system_info->bs_mac, air_msg->dest_mac_addr) != 0){
				zlog_info(zlog_handler," check dest MAC fail , not to my air frame");
				break;
			}

			/* check  duplicate air frame */
			if(checkAirFrameDuplicate(MSG_RECEIVED_ASSOCIATION_RESPONSE, g_system_info)){
				zlog_info(zlog_handler, "duplicated association response air frame !!!!!!!! ");
				break;
			}

			// need check system state
			if(g_system_info->bs_state == STATE_INIT_SELECTED){
				send_initcompleted_signal(g_network->node->my_id, g_network->node->system_info->ve_id, g_network);
				trigger_mac_id(g_RegDev); 
				open_ddr(g_RegDev);
				g_system_info->bs_state = STATE_WORKING;
				zlog_info(zlog_handler," SYSTEM STATE CHANGE : bs state STATE_INIT_SELECTED -> STATE_WORKING");
		
				printf(" SYSTEM STATE CHANGE : bs state STATE_INIT_SELECTED -> STATE_WORKING\n");
				printf(" ---------------------- B1 Events completed --------------------------\n");


			}else if(g_system_info->bs_state == STATE_TARGET_SELECTED){

				trigger_mac_id(g_RegDev);
				open_ddr(g_RegDev);

				g_system_info->bs_state = STATE_WORKING;
				zlog_info(zlog_handler," SYSTEM STATE CHANGE : bs state STATE_TARGET_SELECTED -> STATE_WORKING");

				send_linkopen_signal(g_network->node->my_id, g_network->node->system_info->ve_id, g_network);

			}

			/* ------- distance air frame , working bs start to measure distance -- 20191023 ---------*/
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
			send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
			postCheckWorkToThreadPool(DISTANC_MEASURE_REQUEST, g_msg_queue, g_air, g_threadpool);

			uint32_t delay =(g_system_info->my_initial + g_system_info->other_initial)/2+32;
			set_delay_tick(g_RegDev,delay);

			// source monitor itself
			postMonitorWorkToThreadPool(g_network->node, g_msg_queue, g_network, g_RegDev, g_threadpool, 4);

			/* --------------------------------------------------------------------------------------- */
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_RESPONSE:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_HANDOVER_START_RESPONSE: msg_number = %d ", getData->msg_number);

			/* check dest mac */
			if(is_my_air_frame(g_system_info->bs_mac, air_msg->dest_mac_addr) != 0){
				zlog_info(zlog_handler," check dest MAC fail , not to my air frame");
				break;
			}

			/* check  duplicate air frame */
			if(checkAirFrameDuplicate(MSG_RECEIVED_HANDOVER_START_RESPONSE, g_system_info)){
				zlog_info(zlog_handler, "duplicated handover start response air frame !!!!!!!! ");
				break;
			}
	
			/* 3. send DEASSOCIATION via air */
	
			// check rx eth filter data
			zlog_info(zlog_handler,"before close_ddr : rx_byte_filter_ether_low32() = %x \n", rx_byte_filter_ether_low32(g_RegDev));
			zlog_info(zlog_handler,"rx_byte_filter_ether_high32() = %x \n", rx_byte_filter_ether_high32(g_RegDev));

			//zlog_info(zlog_handler,"point 1 :ddr_empty = %u \n",ddr_empty_flag(g_RegDev));
			usleep(1000); // to empty ddr
			close_ddr(g_RegDev);
			//zlog_info(zlog_handler,"point 2 :ddr_empty = %u \n",ddr_empty_flag(g_RegDev));
			usleep(1000);
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			postCheckWorkToThreadPool(DEASSOCIATION, g_msg_queue, g_air, g_threadpool);

			break;
		}
		case MSG_RECEIVED_REASSOCIATION:// !!!!!!!!!!!!!!!!!!!!!!!! ve start to connect target bs
		{

			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_REASSOCIATION: msg_number = %d ", getData->msg_number);

			if(g_system_info->have_ve_mac == 0){
				memcpy(g_system_info->ve_mac,air_msg->source_mac_addr, 6);
				g_system_info->have_ve_mac = 1;
				configureDstMacToBB(g_system_info->ve_mac,g_RegDev,zlog_handler);
				//zlog_info(zlog_handler," REASSOCIATION receive ve_mac = : ");
				//hexdump(g_system_info->ve_mac, 6);
			}

			// beacon and reassocication carry initial value -- 20191023
			if(g_system_info->have_other_initial == 0){
				uint32_t other_inital = 0;
				memcpy((char*)(&other_inital), air_msg->Next_dest_mac_addr, sizeof(uint32_t));
				g_system_info->other_initial = other_inital;
				g_system_info->have_other_initial = 1;

				uint8_t ve_id = 0;
				memcpy((char*)(&ve_id), air_msg->Next_dest_mac_addr + sizeof(uint32_t), sizeof(uint8_t));
				g_system_info->ve_id = (int32_t)ve_id;
			}

			/* check dest mac and state */
			if(g_system_info->bs_state == STATE_WAIT_MONITOR){ // for target bs and other waiting bs
				if(is_my_air_frame(g_system_info->bs_mac, air_msg->dest_mac_addr) != 0){// waiting bs
					printf(" 0415 debug -------- waiting bs \n");
					printf("msgJsonDestMac(getData->msg_json) = : ");
					hexdump(air_msg->dest_mac_addr, 6);
					break;
				}

				g_system_info->received_reassociation = 1;
				if(g_system_info->sourceBs_dac_disabled == 0){
					break;				
				}

				// for target bs
				postMsgWrapper(MSG_TARGET_BS_START_WORKING, NULL, 0, g_x2->g_msg_queue);

				
			}else if(g_system_info->bs_state == STATE_WORKING){ // for source bs

				/* check  duplicate air frame */
				if(checkAirFrameDuplicate(MSG_RECEIVED_REASSOCIATION, g_system_info)){
					zlog_info(zlog_handler, "duplicated reassociation air frame in source bs !!!!!!!! ");
					break;
				}

				disable_dac(g_RegDev);
				// inform target bs , dac is closed
				send_dac_closed_x2_signal(g_network->node->my_id, g_network->node->udp_server_ip, g_x2);
				postCheckNetworkSignalWorkToThreadPool(MSG_RECEIVED_DAC_CLOSED_ACK, g_msg_queue, g_threadpool);				
				
				reset_bb(g_RegDev);
				release_bb(g_RegDev);

				send_linkclosed_signal(g_network->node->my_id, g_network->node->system_info->ve_id, g_network);
				g_system_info->bs_state = STATE_WAIT_MONITOR;
				zlog_info(zlog_handler," SYSTEM STATE CHANGE : bs state STATE_WORKING -> STATE_WAIT_MONITOR");

				g_system_info->have_other_initial = 0;

			}else
				zlog_info(zlog_handler," debug error----STATE_TARGET_SELECTED ? ---- g_system_info->bs_state = %d \n", g_system_info->bs_state);

			break;
		}
		case MSG_RECEIVED_DISTANC_MEASURE_REQUEST:
		{
			//zlog_info(zlog_handler," -------- EVENT : MSG_RECEIVED_DISTANC_MEASURE_REQUEST: msg_number = %d ", getData->msg_number);

			break;
		}
		case MSG_RECEIVED_KEEP_ALIVE:
		{
			g_system_info->received_air_state_list->received_keepAlive = 1;
			if(g_system_info->bs_state == STATE_WAIT_MONITOR){ // for target bs and other waiting bs
				uint8_t ve_id = 0;
				memcpy((char*)(&ve_id), air_msg->Next_dest_mac_addr + sizeof(uint32_t), sizeof(uint8_t));
				g_system_info->ve_id = (int32_t)ve_id;
			}else if(g_system_info->bs_state == STATE_WORKING){ // for source bs
				send_airSignal(KEEP_ALIVE_ACK, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			}
			break;
		}
		default:
			break;
	}
	free(air_msg);
}

/* self event */
void process_self_event(struct msg_st* getData, g_network_para* g_network, g_air_para* g_air, 
                        g_x2_para* g_x2, g_RegDev_para* g_RegDev, g_msg_queue_para* g_msg_queue, 
				        ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_network->node->system_info;
	switch(getData->msg_type){
		case MSG_CHECK_RECEIVED_LIST:
		{
			int32_t subtype = -1;
			memcpy((char*)(&subtype), getData->msg_json, getData->msg_len);
			if(subtype == ASSOCIATION_REQUEST)
				zlog_info(zlog_handler," --- ASSOCIATION_REQUEST ------- EVENT : MSG_CHECK_RECEIVED_LIST: msg_number = %d", getData->msg_number);
			else if(subtype == HANDOVER_START_REQUEST)
				zlog_info(zlog_handler," --- HANDOVER_START_REQUEST ----- EVENT : MSG_CHECK_RECEIVED_LIST: msg_number = %d", getData->msg_number);
			else if(subtype == DEASSOCIATION)
				zlog_info(zlog_handler," --- DEASSOCIATION ----- EVENT : MSG_CHECK_RECEIVED_LIST: msg_number = %d", getData->msg_number);
			// check receive list according to subtype
			checkReceivedList(subtype, g_system_info, g_msg_queue, g_air, g_threadpool);
			break;
		}
		case MSG_TIMEOUT:
		{

			zlog_info(zlog_handler," ---------------- EVENT : MSG_TIMEOUT: msg_number = %d",getData->msg_number);

			break;
		}
		case MSG_START_HANDOVER_THROUGH_AIR:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_HANDOVER_THROUGH_AIR: msg_number = %d",getData->msg_number);
			
			/* 1. air_interface send Handover start request (Next_dest_mac_addr set target) */
			send_airSignal(HANDOVER_START_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->next_bs_mac, g_air);
			//memset(g_system_info->next_bs_mac,0,6); // note in checkRecivedList
			postCheckWorkToThreadPool(HANDOVER_START_REQUEST, g_msg_queue, g_air, g_threadpool);
			break;
		}
		case MSG_TARGET_BS_START_WORKING:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_TARGET_BS_START_WORKING: msg_number = %d",getData->msg_number);

			enable_dac(g_RegDev);

			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t)); // 20191024
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, my_inital, g_air);
			postCheckWorkToThreadPool(ASSOCIATION_REQUEST, g_msg_queue, g_air, g_threadpool);
			g_system_info->bs_state = STATE_TARGET_SELECTED;
			g_system_info->received_reassociation = 0; // clear status
			g_system_info->sourceBs_dac_disabled = 0; // clear status  
			zlog_info(zlog_handler," SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_TARGET_SELECTED");
			break;	
		}
		case MSG_MONITOR_READY_HANDOVER:
		{
            zlog_info(zlog_handler," ---------------- EVENT : MSG_MONITOR_READY_HANDOVER: msg_number = %d",getData->msg_number);
			int32_t quility = -1;
			memcpy((char*)(&quility), getData->msg_json, getData->msg_len);
			send_ready_handover_signal(g_network->node->my_id, g_network->node->my_mac_str, quility, g_network);
			g_system_info->monitored = 0;
			break;
        }
		case MSG_CHECK_RECEIVED_NETWORK_LIST:
		{
			int32_t signal = -1;
			memcpy((char*)(&signal), getData->msg_json, getData->msg_len);
			if(signal == MSG_RECEIVED_DAC_CLOSED_ACK)
				zlog_info(zlog_handler," --- dac closed ------- EVENT : MSG_CHECK_RECEIVED_NETWORK_LIST: msg_number = %d", getData->msg_number);
			// check receive list according to subtype
			checkNetWorkReceivedList(signal, g_system_info, g_msg_queue, g_x2, g_threadpool);
			break;
		}
		case MSG_SOURCE_BS_DISTANCE_NEAR:
		{
			zlog_info(zlog_handler," -------- EVENT : MSG_SOURCE_BS_DISTANCE_NEAR: msg_number = %d ", getData->msg_number);
			g_system_info->distance_near_confirm = 1;
			if(g_system_info->received_handover_start_confirm == 0){
				break;
			}

			// start handover progress .....
			postMsgWrapper(MSG_CONFIRM_START_HANDOVER, NULL, 0, g_network->g_msg_queue);

			break;
		}
		case MSG_CONFIRM_START_HANDOVER:
		{
			zlog_info(zlog_handler," -------- EVENT : MSG_CONFIRM_START_HANDOVER: msg_number = %d ", getData->msg_number);

			g_system_info->handover_cnt = g_system_info->handover_cnt + 1;
			printf("receive MSG_CONFIRM_START_HANDOVER ..............-----------------.......handover_cnt =  %d \n", g_system_info->handover_cnt);

			if(g_network->node->enable_user_wait == 1)
				user_wait(); // hold on to start handover : move to configure node parameter
			else{
				for(int i = 0;i<g_network->node->sleep_cnt_second;i++)
					gw_sleep();
			}
			
			g_system_info->distance_near_confirm = 0; // clear status
			g_system_info->received_handover_start_confirm = 0; // clear status 

			// sync data
			send_change_tunnel_signal(g_network->node->my_id, g_network->node->system_info->ve_id, g_network);
			postCheckTxBufferWorkToThreadPool(g_network->node, g_msg_queue, g_RegDev, g_threadpool);

			break;
		}
		default:
			break;
	}
}

void init_state(g_air_para* g_air, g_network_para* g_network, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	system_info_para* g_system_info = g_network->node->system_info;
	reset_bb(g_RegDev);
	/* init src_mac */
	//int ret = set_src_mac_fast(g_RegDev, g_system_info->bs_mac);
	disable_dac(g_RegDev);
	close_ddr(g_RegDev);
	release_bb(g_RegDev);

	// get initial tick value
	enable_dac(g_RegDev);
	Enter_self_test(g_air,g_RegDev); // nedd open dac
	disable_dac(g_RegDev);
}

void eventLoop(g_network_para* g_network, g_air_para* g_air, g_x2_para* g_x2, g_msg_queue_para* g_msg_queue, 
               g_RegDev_para* g_RegDev, ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{

	init_state(g_air, g_network, g_RegDev,zlog_handler);

	zlog_info(zlog_handler," ------------------------------ start baseStation event loop ----------------------------- \n");

	while(1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;

		if(getData->msg_type < MSG_START_MONITOR)
			process_air_event(getData, g_network, g_air, g_x2, g_RegDev, g_msg_queue, g_threadpool, zlog_handler);
		else if(getData->msg_type < MSG_TIMEOUT)
			process_network_event(getData, g_network, g_air, g_x2, g_RegDev, g_msg_queue, g_threadpool, zlog_handler);
		else if(getData->msg_type >= MSG_TIMEOUT)
			process_self_event(getData, g_network, g_air, g_x2, g_RegDev, g_msg_queue, g_threadpool, zlog_handler);

		free(getData);
	}
}







