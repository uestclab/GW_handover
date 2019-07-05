#ifndef VEHICLE_COMMON_H
#define VEHICLE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct system_info_para{
	int32_t     ve_state; // indicate system state while time flow
	char        link_bs_mac[6];
	char        ve_mac[6];
	char        next_bs_mac[6]; // send REASSOCIATION as dst
	int         isLinked;
	uint16_t    send_id;
	uint16_t    rcv_id;    
}system_info_para;


typedef struct ConfigureNode{
	system_info_para*  system_info;
	int32_t vehicle_id;
	char* my_mac_str;
	char* my_Ethernet;
}ConfigureNode;

// test msg type
#define MSG_NETWORK 1
#define MSG_AIR     2
#define MSG_MONITOR 3


// system state 

#define STATE_STARTUP           0
#define STATE_SYSTEM_READY      1
#define STATE_WORKING           2
#define STATE_HANDOVER          3


#define MSG_RECEIVED_ASSOCIATION_REQUEST      10
#define MSG_RECEIVED_DEASSOCIATION            11
#define MSG_RECEIVED_HANDOVER_START_REQUEST   12
 
#define MSG_STARTUP                           20


#endif//VEHICLE_COMMON_H
