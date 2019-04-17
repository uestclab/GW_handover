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
	zlog_info(zlog_handler," start set_dst_mac_fast \n");
	set_dst_mac_fast(g_RegDev, link_bs_mac);
	zlog_info(zlog_handler," end set_dst_mac_fast \n");
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

			zlog_info(zlog_handler," printf(link_bs_mac = : \n); ************************ printf \n");
			//printf("link_bs_mac = : \n");
			//hexdump(g_system_info->link_bs_mac, 6);

			send_airSignal(ASSOCIATION_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_periodic->g_air);

			/* configure link_bs_mac to BB */
			configureDstMacToBB(g_system_info->link_bs_mac,g_RegDev,zlog_handler);
			g_system_info->isLinked = 1;
			g_system_info->ve_state = STATE_WORKING;

			open_ddr(g_RegDev); // for init bs or for target bs
			
			trigger_mac_id(g_RegDev); // for target bs . or init bs needed?  			

			zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : ve state STATE_SYSTEM_READY -> STATE_WORKING");
			printf(" ************************* SYSTEM STATE CHANGE : ve state STATE_SYSTEM_READY -> STATE_WORKING \n");

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

// -------------- test point 2: End A2 event  ---------------------------------------------------------------------------------
				
			break;
		}
		case MSG_RECEIVED_HANDOVER_START_REQUEST:
		{
			zlog_info(zlog_handler," ************************************************************************************************** \n");
			zlog_info(zlog_handler," ---------------- EVENT : MSG_RECEIVED_HANDOVER_START_REQUEST: msg_number = %d",getData->msg_number);
			memcpy(g_system_info->next_bs_mac,msgJsonNextDstMac(getData->msg_json), 6);
			send_airSignal(HANDOVER_START_RESPONSE, g_system_info->ve_mac, g_system_info->link_bs_mac, g_system_info->ve_mac, g_periodic->g_air);
	
			/* close ddr */
			close_ddr(g_RegDev);

			g_system_info->ve_state = STATE_HANDOVER;
			zlog_info(zlog_handler," ******* start close_ddr() ********** SYSTEM STATE CHANGE : ve state STATE_WORKING -> STATE_HANDOVER");
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
	zlog_info(zlog_handler," init_state() ----- \n");
	system_info_para* g_system_info = g_periodic->node->system_info;
	
	/* init src_mac */
	zlog_info(zlog_handler," start set_src_mac_fast \n");
	int ret = set_src_mac_fast(g_RegDev, g_system_info->ve_mac);
	zlog_info(zlog_handler,"end set_src_mac_fast : ret = %d \n", ret);
	zlog_info(zlog_handler,"completed set_src_mac_fast ---------- \n");

	disable_dac(g_RegDev);
	close_ddr(g_RegDev);

	/* check system state is ready */
	// while json check system state

	/* open dac */
	enable_dac(g_RegDev);

	g_system_info->ve_state = STATE_SYSTEM_READY;
	zlog_info(zlog_handler," ************************* SYSTEM STATE CHANGE : bs state STATE_STARTUP -> STATE_SYSTEM_READY");

	startProcessAir(g_air, 1); 
	startPeriodic(g_periodic,BEACON); // --------------------------------------- first periodic action
}

void eventLoop(g_air_para* g_air, g_periodic_para* g_periodic, g_msg_queue_para* g_msg_queue, g_RegDev_para* g_RegDev, 
		zlog_category_t* zlog_handler)
{
	char* error_mac = "000000000010";
	char error_bs_mac[6];
	change_mac_buf(error_mac,error_bs_mac);
	//configureDstMacToBB(error_bs_mac,g_RegDev,zlog_handler);
	zlog_info(zlog_handler," configure error dst mac ............. \n");

	init_state(g_air, g_periodic, g_RegDev, zlog_handler);

	zlog_info(zlog_handler," ------------------------------  start vehicle event loop ----------------------------- \n");

	while(1){
		//zlog_info(zlog_handler,"wait getdata ----- \n");
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






