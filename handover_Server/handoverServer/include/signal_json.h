#ifndef GLORY_SIGNAL_JSON_H
#define GLORY_SIGNAL_JSON_H

//#ifdef __cplusplus
//    extern "C" {
//#endif

#include "cJSON.h"
#include "baseStation.h"




void send_id_received_signal(BaseStation* bs,int bs_id);

void send_init_location_signal(BaseStation* bs, int bs_id);

void send_init_link_signal(BaseStation* bs, int bs_id, char* mac);

void send_start_handover_signal(BaseStation* bs, BaseStation* next_bs, int bs_id, char* mac);

void send_change_tunnel_ack_signal(BaseStation* bs, int bs_id);

void send_server_recall_monitor_signal(BaseStation* bs, int bs_id);

// void send_server_recall_monitor_signal(BaseStation* bs, int bs_id, int punish_time);




//#ifdef __cplusplus
//    }
//#endif

#endif  /* ndef GLORY_SIGNAL_JSON_H */
