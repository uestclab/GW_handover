#include "ve_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "gw_frame.h"
#include "ve_utility.h"

// distance measure related function
void self_test_send(g_air_para* g_air){
	system_info_para* g_system_info = g_air->node->system_info;
	 // send DISTANC_MEASURE_REQUEST
	zlog_info(g_air->log_handler,"send DISTANC_MEASURE_REQUEST self_test_send \n");
	for(int i = 0; i < 10 ; i++){
		send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_air);
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



/* ----------------------------------------------------------------- */

int checkAirFrameDuplicate(msg_event receivedAirEvent, system_info_para* g_system_info){
	int flag = 0;
	received_state_list* list = g_system_info->received_air_state_list;
	switch(receivedAirEvent){
		case MSG_RECEIVED_HANDOVER_START_REQUEST:
		{
			if(list->received_handover_start_request == 0){
				list->received_handover_start_request = 1;
			}else{
				flag = 1;
			}			
			break;
		}
		case MSG_RECEIVED_DEASSOCIATION:
		{
			if(list->received_deassociation == 0){
				list->received_deassociation = 1;
			}else{
				flag = 1;
			}			
			break;
		}
		case MSG_RECEIVED_ASSOCIATION_REQUEST:
		{
			if(list->received_association_request == 0){
				list->received_association_request = 1;
			}else{
				flag = 1;
			}			
			break;
		}
	}
	return flag;
}

/* ----------------------------------------------------------------- */

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

uint16_t msgJsonSeqId(char* msg_json){
	uint16_t seq_id = 0;
	memcpy(&seq_id, msg_json + 18, 2);
	return seq_id;
}

void configureDstMacToBB(char* link_bs_mac, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	set_dst_mac_fast(g_RegDev, link_bs_mac);
}

void printMsgType(long int type, uint16_t seq_id){
	if(type == MSG_RECEIVED_ASSOCIATION_REQUEST)
		printf("receive MSG_RECEIVED_ASSOCIATION_REQUEST : seq_id = %d \n", seq_id);
	else if(type == MSG_RECEIVED_DEASSOCIATION)
		printf("receive MSG_RECEIVED_DEASSOCIATION : seq_id = %d \n", seq_id);
	else if(type == MSG_RECEIVED_HANDOVER_START_REQUEST)
		printf("receive MSG_RECEIVED_HANDOVER_START_REQUEST : seq_id = %d \n", seq_id);
}



void process_air_event(struct msg_st* getData, g_air_para* g_air, g_periodic_para* g_periodic, g_RegDev_para* g_RegDev, 
					ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_periodic->node->system_info;

	if(g_system_info->rcv_id == msgJsonSeqId(getData->msg_json)){
		printMsgType(getData->msg_type, msgJsonSeqId(getData->msg_json));
	}
	g_system_info->rcv_id = msgJsonSeqId(getData->msg_json);

	switch(getData->msg_type){
		// start event link ---------------------------------------------------- ve first received event !!!!
		case MSG_RECEIVED_ASSOCIATION_REQUEST: // case in beacon or REASSOCIATION
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_ASSOCIATION_REQUEST: msg_number = %d",getData->msg_number);

			if(g_system_info->isLinked == 1 && g_system_info->ve_state == STATE_WORKING){
				zlog_info(zlog_handler," response more ASSOCIATION_REQUEST \n");
				send_airSignal(ASSOCIATION_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_periodic->g_air);
				break;
			}
			
			stopPeriodic(g_periodic); // stop transmit both BEACON and REASSOCIATION

			// association_request carry initial value -- 20191024
			if(g_system_info->have_other_initial == 0){
				uint32_t other_inital = 0;
				memcpy((char*)(&other_inital), msgJsonNextDstMac(getData->msg_json), sizeof(uint32_t));
				g_system_info->other_initial = other_inital;
				g_system_info->have_other_initial = 1;
				zlog_error(zlog_handler, " ve get other initial value \n");
			}
			// when to start to send DISTANC_MEASURE_REQUEST in vehicle proc? -- 20191024
			g_system_info->new_distance_test_id = g_system_info->new_distance_test_id + 1;
			char my_inital[6];
			memset(my_inital,0x0,6);
			memcpy(my_inital,(char*)(&(g_system_info->my_initial)), sizeof(uint32_t));
			send_airSignal(DISTANC_MEASURE_REQUEST, g_system_info->ve_mac, g_system_info->link_bs_mac, my_inital, g_air);
			postCheckWorkToThreadPool(DISTANC_MEASURE_REQUEST, g_system_info->new_distance_test_id ,g_air->g_msg_queue, g_air, g_threadpool);
			uint32_t delay =(g_system_info->my_initial + g_system_info->other_initial)/2+32;
			set_delay_tick(g_RegDev,delay);
			
			// heart beat start working
			sendAndCheck_keepAlive(g_system_info, g_air, g_threadpool);

			/* configure link_bs_mac to BB */
			memcpy(g_system_info->link_bs_mac,msgJsonSourceMac(getData->msg_json), 6); // 20191024 for initial value transfer 
			configureDstMacToBB(g_system_info->link_bs_mac,g_RegDev,zlog_handler);
			g_system_info->isLinked = 1;
			g_system_info->ve_state = STATE_WORKING;

			send_airSignal(ASSOCIATION_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_periodic->g_air);


			trigger_mac_id(g_RegDev); // for target bs . or init bs needed?  
			open_ddr(g_RegDev); // for init bs or for target bs
				

			zlog_info(zlog_handler,"  SYSTEM STATE CHANGE : ve state STATE_SYSTEM_READY -> STATE_WORKING");

			break;
		}
		case MSG_RECEIVED_DEASSOCIATION:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_DEASSOCIATION: msg_number = %d",getData->msg_number);

			if(g_system_info->isLinked == 0)
				break;

			startPeriodic(g_periodic,REASSOCIATION);// --------------------------------------- second periodic action

			g_system_info->have_other_initial = 0;
			g_system_info->isLinked = 0;
			g_system_info->ve_state = STATE_SYSTEM_READY;			
			
			zlog_info(zlog_handler,"  SYSTEM STATE CHANGE : ve state STATE_HANDOVER -> STATE_SYSTEM_READY");
				
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_REQUEST:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_HANDOVER_START_REQUEST: msg_number = %d",getData->msg_number);

			if(g_system_info->ve_state == STATE_HANDOVER){
				zlog_info(zlog_handler," response more HANDOVER_START_REQUEST \n");
				send_airSignal(HANDOVER_START_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, 
							   g_system_info->ve_mac, g_periodic->g_air);
				break;
			}
				

			memcpy(g_system_info->next_bs_mac,msgJsonNextDstMac(getData->msg_json), 6);
			
			/* close ddr */
			close_ddr(g_RegDev);

			// testdata point
			int time_cnt = 0;
			while(1){
				//zlog_info(zlog_handler,"ve sdram buffer flag = %d ",airdata_buf2_empty_flag(g_RegDev));
				time_cnt = time_cnt + 1;
				if(time_cnt > 3)
					break;
				usleep(500);
			}
			//zlog_info(zlog_handler,"ve sdram buffer flag = %d ",airdata_buf2_empty_flag(g_RegDev));
			send_airSignal(HANDOVER_START_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_periodic->g_air);
	
			g_system_info->ve_state = STATE_HANDOVER;
			zlog_info(zlog_handler,"  SYSTEM STATE CHANGE : ve state STATE_WORKING -> STATE_HANDOVER");
			break;
		}
		case MSG_RECEIVED_DISTANC_MEASURE_REQUEST: // cause : discontinue msg_number 
		{
			//zlog_info(zlog_handler," -------- EVENT : MSG_RECEIVED_DISTANC_MEASURE_REQUEST: msg_number = %d ", getData->msg_number);

			break;
		}
		case MSG_RECEIVED_KEEP_ALIVE_ACK:
		{
			//zlog_info(zlog_handler," -------- EVENT : MSG_RECEIVED_KEEP_ALIVE_ACK: msg_number = %d ", getData->msg_number);
			g_system_info->received_air_state_list->received_keepAlive_ack = 1;
			break;
		}
		default:
			break;
	}
}

void process_self_event(struct msg_st* getData, g_air_para* g_air, g_periodic_para* g_periodic, 
			g_RegDev_para* g_RegDev, ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_periodic->node->system_info;
	switch(getData->msg_type){
		case MSG_CHECK_RECEIVED_LIST:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_CHECK_RECEIVED_LIST: msg_number = %d",getData->msg_number);
			//int32_t subtype = -1;
			//memcpy((char*)(&subtype), getData->msg_json, getData->msg_len);
			retrans_air_t tmp;
			memcpy((char*)(&tmp),getData->msg_json,getData->msg_len);
			// check receive list according to subtype
			if(g_system_info->new_distance_test_id != tmp.id){
				zlog_info(zlog_handler," cancel one distance test : new_id = %d , my_id = %d \n",
							g_system_info->new_distance_test_id, tmp.id);
				break;
			}
			checkReceivedList(tmp.subtype, tmp.id, g_system_info, g_air->g_msg_queue, g_air, g_threadpool);
			break;
		}
		case MSG_ACCESS_FAIL:
		{
			break;
		}
		default:
			break;
	}
}

void init_state(g_air_para* g_air, g_periodic_para* g_periodic, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	system_info_para* g_system_info = g_periodic->node->system_info;
	reset_bb(g_RegDev);
	/* init src_mac */
	int ret = set_src_mac_fast(g_RegDev, g_system_info->ve_mac);
	disable_dac(g_RegDev);
	close_ddr(g_RegDev);
	release_bb(g_RegDev);
	enable_dac(g_RegDev);

	startProcessAir(g_air, 1);
	Enter_self_test(g_air,g_RegDev);
	g_system_info->ve_state = STATE_SYSTEM_READY;
	zlog_info(zlog_handler,"  SYSTEM STATE CHANGE : bs state STATE_STARTUP -> STATE_SYSTEM_READY");
 
	startPeriodic(g_periodic,BEACON); // --------------------------------------- first periodic action
}

void eventLoop(g_air_para* g_air, g_periodic_para* g_periodic, g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, 
		ThreadPool* g_threadpool, zlog_category_t* zlog_handler)
{

	init_state(g_air, g_periodic, g_RegDev, zlog_handler);

	zlog_info(zlog_handler," ------------------------------  start vehicle event loop ----------------------------- \n");

	while(1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;

		if(getData->msg_type < MSG_STARTUP)
			process_air_event(getData, g_air, g_periodic, g_RegDev, g_threadpool, zlog_handler);
		else if(getData->msg_type >= MSG_STARTUP)
			process_self_event(getData, g_air, g_periodic, g_RegDev, g_threadpool, zlog_handler);

		free(getData);
	}
}






