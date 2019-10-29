#ifndef DEFINE_COMMON_H
#define DEFINE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct received_network_list{
	int32_t            received_dac_closed_x2;
	int32_t            received_dac_closed_x2_ack;
}received_network_list;



typedef struct received_state_list{
	int32_t            received_association_response;
	int32_t            received_handover_start_response;
	int32_t            received_reassociation; 
}received_state_list;


typedef struct system_info_para{
	int32_t     bs_state; // indicate system state while time flow
	char        bs_mac[6];
	char        ve_mac[6];
// --------  indicate systme state variable step by step
	char        next_bs_mac[6]; // MSG_START_HANDOVER_THROUGH_AIR use
	int         have_ve_mac;
	int         monitored;
	int         handover_cnt;
// --------  handover signal sequece 
    uint16_t    send_id;
	uint16_t    rcv_id;
// --------  x2 interface 
	int         sourceBs_dac_disabled;
	int         received_reassociation;
// -------- air_signal retransmit 
	received_state_list* received_air_state_list;
// -------- network signal retransmit
	received_network_list* received_network_state_list;
// -------- init tick distance measure
	uint32_t my_initial;
	uint32_t other_initial;
	int      have_my_initial;
	int      have_other_initial;
// -------- handover start confirm
	int      distance_near_confirm;
	int      received_handover_start_confirm;
}system_info_para;

typedef struct ConfigureNode{
	system_info_para*  system_info; // system all info : include state variable
	char* server_ip;
	int32_t server_port;
	int32_t my_id;
	char* my_mac_str;
	char* my_Ethernet;
	//configure delay ms and user_wait
	int   enable_user_wait; //
	int   sleep_cnt_second;
	int   check_eth_rx_cnt;
	int   delay_mon_cnt_second;
	// udp x2 interface
	int32_t udp_server_port;
	char*   udp_server_ip;
	// threadpool parameter
	int32_t task_queue_size;
	int32_t threads_num;
	// ------ distance measure period
	int32_t distance_measure_cnt_ms;
	int     distance_threshold;
	int     snr_threshold;
}ConfigureNode;

typedef enum signalType{
    ID_PAIR = 1,
    ID_RECEIVED,
    READY_HANDOVER,
    INIT_LOCATION,
    INIT_LINK,
    INIT_COMPLETED, // pair_id , relocalization , 
    START_HANDOVER,
    LINK_CLOSED,
    LINK_OPEN,
	CHANGE_TUNNEL,
	CHANGE_TUNNEL_ACK,
	SERVER_RECALL_MONITOR,
}signalType;

typedef struct messageInfo{
    int32_t length;
    signalType signal;
    char* buf; // cjson define payload : signal_json
}messageInfo;

typedef enum msg_event{
	/* test msg type */
    MSG_NETWORK = 1,
	MSG_AIR,
	MSG_MONITOR,
	/* receive air signal , air event */
	MSG_RECEIVED_BEACON,
    MSG_RECEIVED_ASSOCIATION_RESPONSE,
    MSG_RECEIVED_HANDOVER_START_RESPONSE,
    MSG_RECEIVED_REASSOCIATION,
	MSG_RECEIVED_DISTANC_MEASURE_REQUEST,
	/* receive network signal , network event */
    MSG_START_MONITOR, // boundary
    MSG_INIT_SELECTED, 
    MSG_START_HANDOVER,
    MSG_SERVER_RECALL_MONITOR,
    MSG_SOURCE_BS_DAC_CLOSED,
	MSG_RECEIVED_DAC_CLOSED_ACK,
	/* self event */
	MSG_TIMEOUT, // boundary
	MSG_START_HANDOVER_THROUGH_AIR,
	MSG_TARGET_BS_START_WORKING,
	MSG_CHECK_RECEIVED_LIST,
	MSG_MONITOR_READY_HANDOVER,
	MSG_CHECK_RECEIVED_NETWORK_LIST,
	MSG_SOURCE_BS_DISTANCE_NEAR,
	MSG_CONFIRM_START_HANDOVER,
}msg_event;

// used in msg_json
typedef struct air_data{
	char source_mac_addr[6];
	char dest_mac_addr[6];
	char Next_dest_mac_addr[6];
	uint16_t seq_id;
}air_data;

typedef struct net_data{
	char  target_bs_mac[6];
	char  target_bs_ip[32];
}net_data;


// system state 

#define STATE_STARTUP           0
#define STATE_WAIT_MONITOR      1
#define STATE_INIT_SELECTED     2
#define STATE_WORKING           3
#define STATE_TARGET_SELECTED   4




#endif//DEFINE_COMMON_H
