#ifndef BS_NETWORK_JSON_H
#define BS_NETWORK_JSON_H

#include "bs_network.h"
#include "bs_x2.h"

void send_id_pair_signal(int bs_id, char* bs_mac, g_network_para* g_network);
void send_ready_handover_signal(int bs_id, char* bs_mac, int quility, g_network_para* g_network);
void send_initcompleted_signal(int bs_id, g_network_para* g_network);

void send_linkclosed_signal(int bs_id, g_network_para* g_network);
void send_linkopen_signal(int bs_id, g_network_para* g_network);

void send_change_tunnel_signal(int bs_id, g_network_para* g_network);


// ------------- x2 interface -----
void send_dac_closed_x2_signal(int bs_id, g_x2_para* g_x2);
void send_dac_closed_x2_ack_signal(int bs_id, g_x2_para* g_x2);



#endif//BS_NETWORK_JSON_H











