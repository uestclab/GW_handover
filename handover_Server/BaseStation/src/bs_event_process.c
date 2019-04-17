#include "bs_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "bs_network_json.h"
#include "gw_frame.h"
#include "gw_control.h"

#include "timer.h"

typedef struct g_test_para{
	g_RegDev_para* 		g_RegDev;
	g_msg_queue_para* 	g_msg_queue;
	para_thread*        para_t;
	zlog_category_t*    log_handler;
}g_test_para;

struct g_test_para* g_test_all = NULL;

void* check_air_tx_data_statistics(void* args){

	g_RegDev_para*         g_RegDev = g_test_all->g_RegDev;
	zlog_category_t*    log_handler = g_test_all->log_handler;
	zlog_info(log_handler,"Enter check_air_tx_data_statistics()");

	uint32_t decision_cnt = 0;
	uint32_t pre_low32 = rx_byte_filter_ether_low32(g_RegDev);
	uint32_t pre_high32 = rx_byte_filter_ether_high32(g_RegDev);
	zlog_info(log_handler,"rx_byte_filter_ether_low32() = %x \n", pre_low32);
	zlog_info(log_handler,"rx_byte_filter_ether_high32() = %x \n", pre_high32);	

	int wait_cnt = 0;
	while(1)
	{
		usleep(500);
		
		if(rx_byte_filter_ether_low32(g_RegDev) - pre_low32 == 0 && rx_byte_filter_ether_high32(g_RegDev) - pre_high32 == 0){
			decision_cnt = decision_cnt + 1;
			if(decision_cnt > 2){
				zlog_info(log_handler," exit check_air_tx_data_statistics <<<<< no data get in ether, start to air interface handover ! \n");
				break;
			}
		}else{
			pre_low32 = rx_byte_filter_ether_low32(g_RegDev);
			pre_high32 = rx_byte_filter_ether_high32(g_RegDev);
			decision_cnt = 0;
		}
			

		if(wait_cnt == 8)
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

int initCheckTxBufferThread(g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, zlog_category_t* handler)
{
	zlog_info(handler,"initCheckTxBufferThread()");
	if(g_test_all == NULL)
		g_test_all = (struct g_test_para* )malloc(sizeof(struct g_test_para));
	g_test_all->log_handler = handler;
	g_test_all->para_t = newThreadPara();
	g_test_all->g_msg_queue = g_msg_queue;
	g_test_all->g_RegDev = g_RegDev;

	int ret = pthread_create(g_test_all->para_t->thread_pid, NULL, check_air_tx_data_statistics, NULL);
    if(ret != 0){
        zlog_error(handler,"create initCheckTxBufferThread error ! error_code = %d", ret);
		return -1;
    }	
	return 0;
}

// -----------------------------------------------------------------------------------------------------------
static int simulate_cnt = 1;
void simulate_single_trigger_handover(g_network_para* g_network, g_monitor_para* g_monitor){
/*
{
	printf("start to trigger_handover ......... \n");	

	system_info_para* g_system_info = g_network->node->system_info;

	startMonitor(g_monitor,2); // ------- notify bs handover : simulate code , to trigger start handover
	g_system_info->bs_state = STATE_WAIT_MONITOR; // temp change state to simulate source bs and target bs on 1 board currently;
	
	zlog_info(g_network->log_handler," ------ simulate source bs turn to target bs >>>>>>>>>>>>>>>>>>>>>>>>> -------------\n");
}
*/
	{
		system_info_para* g_system_info = g_network->node->system_info;
		printf("start to auto monitor receive quilty ......... simulate cnt = %d \n", simulate_cnt);
		startMonitor(g_monitor,3);
		simulate_cnt = simulate_cnt + 1;
	}

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

			memcpy(g_system_info->next_bs_mac,msgJsonSourceMac(getData->msg_json),6); // temp save next bs mac 
			// need to ensure send status ???????????????? to ensure data sync in 2 different source bs and target bs
			initCheckTxBufferThread(g_network->g_msg_queue, g_RegDev, zlog_handler);
			send_change_tunnel_signal(g_network->node->my_id, g_network);
			
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
			
			if(g_network->node->my_id == 22)
				startMonitor(g_monitor,1); // ------- notify bs start to monitor : simulate code -------------------- first trigger ready_handover
			else if(g_network->node->my_id == 33){			
				StartTimer(timer_cb, NULL, 20, 0, g_air->g_timer);
			}

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
		
				printf(" ************************* SYSTEM STATE CHANGE : bs state STATE_INIT_SELECTED -> STATE_WORKING\n");
				printf(" ---------------------- B1 Events completed --------------------------\n");

// -------------- test point 1 : end test INIT relocation 0411 ----------------------------------------------------------------------------------

			}else if(g_system_info->bs_state == STATE_TARGET_SELECTED){

				open_ddr(g_RegDev);
				trigger_mac_id(g_RegDev);

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
	
			if(g_system_info->received_start_handover_response == 0){
				g_system_info->received_start_handover_response = 1;
			}

			/* 3. send DEASSOCIATION via air */
			close_ddr(g_RegDev);
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			send_airSignal(DEASSOCIATION, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);
			
			/* add read regsiter to ensure DEASSOCIATION transmit successful before disable_dac */

			uint32_t signal_empty_flag = airsignal_buf2_empty_flag(g_RegDev);
			while(signal_empty_flag != 1){
				signal_empty_flag = airsignal_buf2_empty_flag(g_RegDev);
			}		
			zlog_info(zlog_handler," airsignal_is_empty =================================== \n");
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
				if(is_my_air_frame(g_system_info->bs_mac, msgJsonDestMac(getData->msg_json)) != 0){// waiting bs
					zlog_info(zlog_handler," 0415 debug -------- waiting bs ");
					printf("bs_mac = :");
					hexdump(g_system_info->bs_mac, 6);
					printf("msgJsonDestMac(getData->msg_json) = : ");
					hexdump(msgJsonDestMac(getData->msg_json), 6);
					break;
				}

				// for target bs
				enable_dac(g_RegDev);
				send_airSignal(ASSOCIATION_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->bs_mac, g_air);

				g_system_info->bs_state = STATE_TARGET_SELECTED;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WAIT_MONITOR -> STATE_TARGET_SELECTED");

				
			}else if(g_system_info->bs_state == STATE_WORKING){ // for source bs
				send_linkclosed_signal(g_network->node->my_id, g_network);
				g_system_info->bs_state = STATE_WAIT_MONITOR;
				zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_WORKING -> STATE_WAIT_MONITOR");
				
				StartTimer(timer_cb, NULL, 20, 0, g_air->g_timer);

			}else
				zlog_info(zlog_handler," 0415 debug -------- g_system_info->bs_state = %d \n", g_system_info->bs_state);

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

			//printf("manual trriger ready handover ------------------------------- \n");

			//user_wait();

			simulate_single_trigger_handover(g_network, g_monitor);

/*
			if(g_system_info->received_start_handover_response == 0){
				zlog_info(zlog_handler," ---- Not receive start handover response in expect time !!!! ");
			}else{
				
			}
*/
			break;
		}
		case MSG_START_HANDOVER_THROUGH_AIR:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_START_HANDOVER_THROUGH_AIR: msg_number = %d",getData->msg_number);
			
			zlog_info(zlog_handler," !!!!!!!!!!!!!!!!!!! start process disconnect ve and source BS !!!!!!!!!!!!!!!!!!!!!! \n");		
			
			/* 1. air_interface send Handover start request (Next_dest_mac_addr set target) */
			send_airSignal(HANDOVER_START_REQUEST, g_system_info->bs_mac, g_system_info->ve_mac, g_system_info->next_bs_mac, g_air);
			memset(g_system_info->next_bs_mac,0,6);
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






