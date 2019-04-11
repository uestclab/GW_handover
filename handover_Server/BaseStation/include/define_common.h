#ifndef DEFINE_COMMON_H
#define DEFINE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct system_info_para{
	int32_t     bs_state; // indicate system state while time flow
	char        bs_mac[6];
	char        ve_mac[6];
// --------  indicate systme state variable step by step
	int         have_ve_mac;
	int         received_start_handover_response;    
}system_info_para;


typedef struct ConfigureNode{
	system_info_para*  system_info;
	char* server_ip;
	int32_t server_port;
	int32_t my_id;
	char* my_mac_str;
	char* my_Ethernet;
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
}signalType;

typedef struct messageInfo{
    int32_t length;
    signalType signal;
    char* buf; // cjson define payload : signal_json
}messageInfo;


// test msg type
#define MSG_NETWORK 1
#define MSG_AIR     2
#define MSG_MONITOR 3


// event
/*
	receive air signal , air event
*/
#define MSG_RECEIVED_BEACON                         4
#define MSG_RECEIVED_ASSOCIATION_RESPONSE           5
#define MSG_RECEIVED_HANDOVER_START_RESPONSE        6
#define MSG_RECEIVED_REASSOCIATION                  7

/*
	receive network signal , network event
*/
#define MSG_START_MONITOR              10
#define MSG_INIT_SELECTED              11
#define MSG_START_HANDOVER             12


#define MSG_TIMEOUT                    20

// system state 

#define STATE_STARTUP           0
#define STATE_WAIT_MONITOR      1
#define STATE_INIT_SELECTED     2
#define STATE_WORKING           3
#define STATE_TARGET_SELECTED   4




#endif//DEFINE_COMMON_H
