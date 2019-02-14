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
    int32_t   init_num_baseStation;
    char      train_mac_addr[32];
	char      local_ip[32];
	char      script[128];
}serverConfigureNode;


namespace glory
{

enum signalType {
    ID_PAIR = 1,
    ID_RECEIVED,
    READY_HANDOVER,
    INIT_LOCATION,
    INIT_LINK,
    INIT_COMPLETED, // pair_id , relocalization , 
    START_HANDOVER,
    LINK_CLOSED,
    LINK_OPEN,
};

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

enum systemState {PAIR_ID=1, RELOCALIZATION, RUNNING};




}// end namespcae

#endif
