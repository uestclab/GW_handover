#include "bs_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "bs_network_json.h"
#include "gw_frame.h"
#include "gw_control.h"

#include "timer.h"

typedef struct g_test_para{
	ConfigureNode*      node;
	g_RegDev_para* 		g_RegDev;
	g_msg_queue_para* 	g_msg_queue;
	para_thread*        para_t;
	zlog_category_t*    log_handler;
}g_test_para;

struct g_test_para* g_test_all = NULL;

void* check_air_tx_data_statistics(void* args){

	g_RegDev_para*         g_RegDev = g_test_all->g_RegDev;
	zlog_category_t*    log_handler = g_test_all->log_handler;
	
	zlog_info(log_handler,"rx_byte_filter_ether_low32() = %x \n", rx_byte_filter_ether_low32(g_RegDev));
	zlog_info(log_handler,"rx_byte_filter_ether_high32() = %x \n", rx_byte_filter_ether_high32(g_RegDev));
	int wait_cnt = 0;
	while(1)
	{
		usleep(500);
		zlog_info(log_handler,"rx_byte_filter_ether_low32() = %x \n", rx_byte_filter_ether_low32(g_RegDev));
		zlog_info(log_handler,"rx_byte_filter_ether_high32() = %x \n", rx_byte_filter_ether_high32(g_RegDev));
		if(wait_cnt == g_test_all->node->check_eth_rx_cnt) // move to configure node parameter : parameter = 2
			break;
		wait_cnt = wait_cnt + 1;		
	}

	struct msg_st data;
	data.msg_type = MSG_START_HANDOVER_THROUGH_AIR;
	data.msg_number = MSG_START_HANDOVER_THROUGH_AIR;
	data.msg_len = 0;
	postMsgQueue(&data,g_test_all->g_msg_queue);

	destoryThreadPara(g_test_all->para_t);
	free(g_test_all);
	g_test_all = NULL;
}

int initCheckTxBufferThread(struct ConfigureNode* Node, g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, zlog_category_t* handler)
{

	if(g_test_all == NULL)
		g_test_all = (struct g_test_para* )malloc(sizeof(struct g_test_para));
	g_test_all->log_handler = handler;
	g_test_all->para_t = newThreadPara();
	g_test_all->g_msg_queue = g_msg_queue;
	g_test_all->g_RegDev = g_RegDev;
	g_test_all->node = Node;

	int ret = pthread_create(g_test_all->para_t->thread_pid, NULL, check_air_tx_data_statistics, NULL);
    if(ret != 0){
        zlog_error(handler,"create initCheckTxBufferThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

// -----------------------------------------------------------------------------------------------------------

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
	set_dst_mac_fast(g_RegDev, dst_buf);
}

void process_network_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, g_x2_para* g_x2, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
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

			enable_dac(g_RegDev);

			// Next_dest_mac_addr set itself -- first air frame sent by bs ---!
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);

			g_system_info->bs_state = STATE_INIT_SELECTED;
			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_INIT_SELECTED");

			break;
		}
		case MSG_START_HANDOVER: // source bs start to disconnect ve ..
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_HANDOVER: msg_number = %d",getData->msg_number);
			
			g_system_info->handover_cnt = g_system_info->handover_cnt + 1;
			printf("receive MSG_START_HANDOVER ..............-----------------.......handover_cnt =  %d \n", g_system_info->handover_cnt);

			if(g_network->node->enable_user_wait == 1)
				user_wait(); // hold on to start handover : move to configure node parameter
			else{
				for(int i = 0;i<g_network->node->sleep_cnt_second;i++)
					gw_sleep();
			}

			memcpy(g_system_info->next_bs_mac,msgJsonSourceMac(getData->msg_json),6); // temp save next bs mac
			configure_target_ip(NULL, g_x2); 
			// sync data
			send_change_tunnel_signal(g_network->node->my_id, g_network);
			initCheckTxBufferThread(g_network->node, g_network->g_msg_queue, g_RegDev, zlog_handler); // 0418 change seq

			break;
		}
		case MSG_SERVER_RECALL_MONITOR:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_SERVER_RECALL_MONITOR: msg_number = %d",getData->msg_number);
			printf("receive MSG_SERVER_RECALL_MONITOR \n");
			for(int i=0;i<2;i++)
				gw_sleep();
			if(g_system_info->monitored == 0 && g_system_info->bs_state == STATE_WAIT_MONITOR){
				g_system_info->monitored = 1;
				startMonitor(g_monitor,3);
			}else{
				zlog_info(zlog_handler," already in monitor or STATE_WORKING ");
			}
			break;
		}
		case MSG_SOURCE_BS_DAC_CLOSED:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_SOURCE_BS_DAC_CLOSED: msg_number = %d",getData->msg_number);
			
			g_system_info->sourceBs_dac_disabled = 1;
			if(g_system_info->received_reassociation == 0){
				break;				
			}

			// for target bs
			struct msg_st data;
			data.msg_type = MSG_TARGET_BS_START_WORKING;
			data.msg_number = MSG_TARGET_BS_START_WORKING;
			data.msg_len = 0;
			postMsgQueue(&data,g_x2->g_msg_queue);
			break;
		}
		default:
			break;
	}	
}

void process_air_event(struct msg_st* getData, g_network_para* g_network, g_monitor_para* g_monitor, 
				g_air_para* g_air, g_x2_para* g_x2, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
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
			
			if(g_network->node->my_id == 11)
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
				trigger_mac_id(g_RegDev); // 0418 add 
				open_ddr(g_RegDev);
				g_system_info->bs_state = STATE_WORKING;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_INIT_SELECTED -> STATE_WORKING");
		
				printf(" ************************* SYSTEM STATE CHANGE : bs state STATE_INIT_SELECTED -> STATE_WORKING\n");
				printf(" ---------------------- B1 Events completed --------------------------\n");


			}else if(g_system_info->bs_state == STATE_TARGET_SELECTED){

				trigger_mac_id(g_RegDev);
				open_ddr(g_RegDev);

				g_system_info->bs_state = STATE_WORKING;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_TARGET_SELECTED -> STATE_WORKING");

				send_linkopen_signal(g_network->node->my_id, g_network);

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
	
			/* 3. send DEASSOCIATION via air */
	
			// check rx eth filter data
			zlog_info(zlog_handler,"before close_ddr : rx_byte_filter_ether_low32() = %x \n", rx_byte_filter_ether_low32(g_RegDev));
			zlog_info(zlog_handler,"rx_byte_filter_ether_high32() = %x \n", rx_byte_filter_ether_high32(g_RegDev));

			zlog_info(zlog_handler,"point 1 :ddr_empty = %u \n",ddr_empty_flag(g_RegDev));
			usleep(1000); // to empty ddr
			close_ddr(g_RegDev);
			zlog_info(zlog_handler,"point 2 :ddr_empty = %u \n",ddr_empty_flag(g_RegDev));
			usleep(1000);
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			//send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);

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
				//hexdump(g_system_info->ve_mac, 6);
			}

			/* check dest mac and state */
			if(g_system_info->bs_state == STATE_WAIT_MONITOR){ // for target bs and other waiting bs
				if(is_my_air_frame(g_system_info->bs_mac, msgJsonDestMac(getData->msg_json)) != 0){// waiting bs
					printf(" 0415 debug -------- waiting bs \n");
					printf("msgJsonDestMac(getData->msg_json) = : ");
					hexdump(msgJsonDestMac(getData->msg_json), 6);
					break;
				}

				//usleep(3000); // wait source bs disable
				g_system_info->received_reassociation = 1;
				if(g_system_info->sourceBs_dac_disabled == 0){
					break;				
				}

				// for target bs

				struct msg_st data;
				data.msg_type = MSG_TARGET_BS_START_WORKING;
				data.msg_number = MSG_TARGET_BS_START_WORKING;
				data.msg_len = 0;
				postMsgQueue(&data,g_x2->g_msg_queue);

				
			}else if(g_system_info->bs_state == STATE_WORKING){ // for source bs

				disable_dac(g_RegDev);

				send_dac_closed_x2_signal(g_network->node->my_id, g_x2); // inform target bs , dac is closed

				reset_bb(g_RegDev);

				send_linkclosed_signal(g_network->node->my_id, g_network);
				g_system_info->bs_state = STATE_WAIT_MONITOR;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WORKING -> STATE_WAIT_MONITOR");

			}else
				zlog_info(zlog_handler," 0415 debug ----STATE_TARGET_SELECTED ? ---- g_system_info->bs_state = %d \n", g_system_info->bs_state);

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

			break;
		}
		case MSG_START_HANDOVER_THROUGH_AIR:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_HANDOVER_THROUGH_AIR: msg_number = %d",getData->msg_number);
			
			/* 1. air_interface send Handover start request (Next_dest_mac_addr set target) */
			send_airSignal(HANDOVER_START_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->next_bs_mac, g_air);
			memset(g_system_info->next_bs_mac,0,6);
			break;
		}
		case MSG_TARGET_BS_START_WORKING:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_TARGET_BS_START_WORKING: msg_number = %d",getData->msg_number);

			enable_dac(g_RegDev);
			send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);

			g_system_info->bs_state = STATE_TARGET_SELECTED;
			g_system_info->received_reassociation = 0; // clear status
			g_system_info->sourceBs_dac_disabled = 0; // clear status  
			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_TARGET_SELECTED");
			break;	
		}
		default:
			break;
	}
}

void init_state(g_network_para* g_network, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	system_info_para* g_system_info = g_network->node->system_info;
	
	/* init src_mac */
	int ret = set_src_mac_fast(g_RegDev, g_system_info->bs_mac);
	disable_dac(g_RegDev);
	close_ddr(g_RegDev);
}

void eventLoop(g_network_para* g_network, g_monitor_para* g_monitor, g_air_para* g_air, g_x2_para* g_x2, 
    g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
{

	init_state(g_network, g_RegDev,zlog_handler);

	zlog_info(zlog_handler," ------------------------------  start baseStation event loop ----------------------------- \n");

	while(1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;

		if(getData->msg_type < MSG_START_MONITOR)
			process_air_event(getData, g_network, g_monitor, g_air, g_x2, g_RegDev, zlog_handler);
		else if(getData->msg_type < MSG_TIMEOUT)
			process_network_event(getData, g_network, g_monitor, g_air, g_x2, g_RegDev, zlog_handler);
		else if(getData->msg_type >= MSG_TIMEOUT)
			process_self_event(getData, g_network, g_monitor, g_air, g_RegDev, zlog_handler);

		free(getData);
	}
}






