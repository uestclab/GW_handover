#ifndef DEFINE_COMMON_H
#define DEFINE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct ConfigureNode{
	char* server_ip;
	int32_t server_port;
	int32_t my_id;
	char* my_mac;
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

typedef struct signal_json{
    int32_t bsId_;
    double rssi_;
    char bsMacAddr_[32];
    char trainMacAddr_[32];
}signal_json;

typedef struct messageInfo{
    int32_t length;
    signalType signal;
    char* buf; // cjson define payload : signal_json
}messageInfo;


#endif//DEFINE_COMMON_H
