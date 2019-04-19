#include "ve_event_process.h"
#include "zlog.h"
#include "cJSON.h"
#include "gw_frame.h"

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

void configureDstMacToBB(char* link_bs_mac, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	set_dst_mac_fast(g_RegDev, link_bs_mac);
}

void process_air_event(struct msg_st* getData, g_air_para* g_air, g_periodic_para* g_periodic, g_RegDev_para* g_RegDev, 
					zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_periodic->node->system_info;
	switch(getData->msg_type){
		// start event link ---------------------------------------------------- ve first received event !!!!
		case MSG_RECEIVED_ASSOCIATION_REQUEST: // case in beacon or REASSOCIATION
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_ASSOCIATION_REQUEST: msg_number = %d",getData->msg_number);
			if(g_system_info->isLinked == 1 && g_system_info->ve_state == STATE_WORKING){
				break;
			}
			
			stopPeriodic(g_periodic); // stop transmit both BEACON and REASSOCIATION

			memcpy(g_system_info->link_bs_mac,msgJsonNextDstMac(getData->msg_json), 6);

			send_airSignal(ASSOCIATION_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_periodic->g_air);

			/* configure link_bs_mac to BB */
			configureDstMacToBB(g_system_info->link_bs_mac,g_RegDev,zlog_handler);
			g_system_info->isLinked = 1;
			g_system_info->ve_state = STATE_WORKING;

			//check if ddr is full
			if(ddr_full_flag(g_RegDev) != 0){
				zlog_info(zlog_handler," ddr is full !!!!!!!!!!!! %u ", ddr_full_flag(g_RegDev));
				//reset_ddr_full_flag(g_RegDev);
			}

			trigger_mac_id(g_RegDev); // for target bs . or init bs needed?  
			open_ddr(g_RegDev); // for init bs or for target bs					

			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : ve state STATE_SYSTEM_READY -> STATE_WORKING");

			break;
		}
		case MSG_RECEIVED_DEASSOCIATION:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_DEASSOCIATION: msg_number = %d",getData->msg_number);
			if(g_system_info->isLinked == 0)
				break;

			startPeriodic(g_periodic,REASSOCIATION);// --------------------------------------- second periodic action

			g_system_info->isLinked = 0;
			g_system_info->ve_state = STATE_SYSTEM_READY;			
			
			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : ve state STATE_HANDOVER -> STATE_SYSTEM_READY");
				
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_REQUEST:
		{
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_HANDOVER_START_REQUEST: msg_number = %d",getData->msg_number);
			memcpy(g_system_info->next_bs_mac,msgJsonNextDstMac(getData->msg_json), 6);
			
			/* close ddr */
			close_ddr(g_RegDev);
			// testdata point
			int time_cnt = 0;
			while(1){
				zlog_info(zlog_handler,"ve sdram buffer flag = %d ",airdata_buf2_empty_flag(g_RegDev));
				time_cnt = time_cnt + 1;
				if(time_cnt > 4)
					break;
				usleep(500);
			}

			send_airSignal(HANDOVER_START_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_periodic->g_air);
	
			g_system_info->ve_state = STATE_HANDOVER;
			zlog_info(zlog_handler," ************************ SYSTEM STATE CHANGE : ve state STATE_WORKING -> STATE_HANDOVER");
			break;
		}
		default:
			break;
	}
}

void process_self_event(struct msg_st* getData, g_air_para* g_air, g_periodic_para* g_periodic, 
			g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler)
{
	system_info_para* g_system_info = g_periodic->node->system_info;
	switch(getData->msg_type){
		default:
			break;
	}
}

void init_state(g_air_para* g_air, g_periodic_para* g_periodic, g_RegDev_para* g_RegDev, zlog_category_t* zlog_handler){
	system_info_para* g_system_info = g_periodic->node->system_info;
	
	/* init src_mac */
	int ret = set_src_mac_fast(g_RegDev, g_system_info->ve_mac);
	disable_dac(g_RegDev);
	close_ddr(g_RegDev);

	enable_dac(g_RegDev);
	g_system_info->ve_state = STATE_SYSTEM_READY;
	zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_STARTUP -> STATE_SYSTEM_READY");

	startProcessAir(g_air, 1); 
	startPeriodic(g_periodic,BEACON); // --------------------------------------- first periodic action
}

void eventLoop(g_air_para* g_air, g_periodic_para* g_periodic, g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, 
		zlog_category_t* zlog_handler)
{

	init_state(g_air, g_periodic, g_RegDev, zlog_handler);

	zlog_info(zlog_handler," ------------------------------  start vehicle event loop ----------------------------- \n");

	while(1){
		struct msg_st* getData = getMsgQueue(g_msg_queue);
		if(getData == NULL)
			continue;

		if(getData->msg_type < MSG_STARTUP)
			process_air_event(getData, g_air, g_periodic, g_RegDev, zlog_handler);
		else if(getData->msg_type >= MSG_STARTUP)
			process_self_event(getData, g_air, g_periodic, g_RegDev, zlog_handler);

		free(getData);
	}
}






