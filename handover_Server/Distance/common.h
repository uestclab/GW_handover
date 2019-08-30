#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct received_state_list{
	int32_t            received_delay_exchange_response;
}received_state_list;

typedef struct system_info_para{
	char        bs_mac[6];
	char        ve_mac[6];
// --------  indicate systme state variable step by step
	char        next_bs_mac[6]; // MSG_START_HANDOVER_THROUGH_AIR use
	int         have_ve_mac;
// --------  handover signal sequece 
    uint16_t    send_id;
	uint16_t    rcv_id;
// -------- init tick
	uint32_t my_initial;
	uint32_t other_initial;
	int      have_my_initial;
	int      have_other_initial;
// -------- check retransmit
	received_state_list* received_air_state_list;
}system_info_para;

typedef struct ConfigureNode{
	system_info_para*  system_info; // system all info : include state variable
	int32_t my_id;
	char* my_mac_str;
	char* my_Ethernet;
	int32_t measure_cnt_ms;
}ConfigureNode;


// used in msg_json
typedef struct air_data{
	char source_mac_addr[6];
	char dest_mac_addr[6];
	char Next_dest_mac_addr[6];
	uint16_t seq_id;
}air_data;

#define DISTANC_MEASURE_REQUEST    9
#define DELAY_EXCHANGE_REQUEST     10
#define DELAY_EXCHANGE_RESPONSE    13

typedef enum msg_event{
	MSG_CHECK_RECEIVED_LIST = 1,
	MSG_INIT_TICK_ONCE_MORE,
	MSG_RECEIVED_DISTANC_MEASURE_REQUEST,
	MSG_RECEIVED_DELAY_EXCHANGE_REQUEST,
	MSG_RECEIVED_DELAY_EXCHANGE_RESPONSE,
	MSG_CACULATE_DISTANCE,
}msg_event;

typedef struct g_air_para{
	int                running;
	g_msg_queue_para*  g_msg_queue;
    ConfigureNode*     node;
	para_thread*       para_t;
	para_thread*       send_para_t;
	zlog_category_t*   log_handler;
}g_air_para;

typedef struct retrans_air_t{
	int32_t subtype;
	g_msg_queue_para* g_msg_queue;
}retrans_air_t;

/* global variable */
typedef struct global_var_t{
	zlog_category_t*   log_handler;
	int32_t program_run;
	g_msg_queue_para* g_msg_queue;
}global_var_t;





#endif//COMMON_H
