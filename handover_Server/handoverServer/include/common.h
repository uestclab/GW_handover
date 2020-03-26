#ifndef GLORY_COMMON_H
#define GLORY_COMMON_H

using namespace std;

typedef struct serverConfigureNode{
    int32_t   listen_port;
    int32_t   num_baseStation;
    int32_t   max_listen_backlog;
    int32_t   timer_duration;
    int32_t   water_max_read;
    int32_t   water_low_read;
	char      local_ip[32];
	char      my_Ethernet[32];
	char      script[128];
}serverConfigureNode;


namespace glory
{

enum signalType {
    ID_PAIR = 1,
    ID_RECEIVED,
    BEACON_RECV,
    INIT_DISTANCE,
    INIT_DISTANCE_OVER,
    READY_HANDOVER,
    INIT_LOCATION,
    INIT_LINK,
    INIT_COMPLETED, // pair_id  , 
    START_HANDOVER,
    LINK_CLOSED,
    LINK_OPEN,
	CHANGE_TUNNEL,
	CHANGE_TUNNEL_ACK,
	SERVER_RECALL_MONITOR,
};

typedef struct messageInfo{
    int32_t length;
    signalType signal;
    char* buf; // cjson define payload : signal_json
}messageInfo;

enum systemState {PAIR_ID=1, RELOCALIZATION, RUNNING};


}// end namespcae

#endif
