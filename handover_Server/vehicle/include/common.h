#ifndef VEHICLE_COMMON_H
#define VEHICLE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct received_state_list{
	int32_t            received_association_request;
	int32_t            received_handover_start_request;
	int32_t            received_deassociation; 
}received_state_list;

typedef struct system_info_para{
	int32_t     ve_state; // indicate system state while time flow
	char        link_bs_mac[6];
	char        ve_mac[6];
	char        next_bs_mac[6]; // send REASSOCIATION as dst
	int         isLinked;
// --------  handover signal sequece 
	uint16_t    send_id;
	uint16_t    rcv_id;
// -------- air_signal retransmit 
	received_state_list* received_air_state_list;
// -------- init tick distance measure
	uint32_t my_initial;
	uint32_t other_initial;
	int      have_my_initial;
	int      have_other_initial;    
}system_info_para;


typedef struct ConfigureNode{
	system_info_para*  system_info;
	int32_t vehicle_id;
	char* my_mac_str;
	char* my_Ethernet;
// ------ distance measure period
	int32_t distance_measure_cnt_ms;
	int     distance_threshold;
}ConfigureNode;

// system state 

#define STATE_STARTUP           0
#define STATE_SYSTEM_READY      1
#define STATE_WORKING           2
#define STATE_HANDOVER          3

typedef enum msg_event{
	/* test msg type */
    MSG_NETWORK = 1,
	MSG_AIR,
	MSG_MONITOR,
	/* receive air signal , air event */
	MSG_RECEIVED_ASSOCIATION_REQUEST,
    MSG_RECEIVED_DEASSOCIATION,
    MSG_RECEIVED_HANDOVER_START_REQUEST,
	MSG_RECEIVED_DISTANC_MEASURE_REQUEST,
	/* self event */
	MSG_STARTUP,
	MSG_CHECK_RECEIVED_LIST,
}msg_event;


#endif//VEHICLE_COMMON_H

