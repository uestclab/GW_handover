#ifndef DEFINE_COMMON_H
#define DEFINE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct system_info_para{
	int32_t     bs_state; // indicate system state while time flow
	char        bs_mac[6];
	char        ve_mac[6];
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


#endif//DEFINE_COMMON_H
