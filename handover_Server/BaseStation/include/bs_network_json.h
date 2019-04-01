#ifndef BS_NETWORK_JSON_H
#define BS_NETWORK_JSON_H

#include "bs_network.h"

void send_id_pair_signal(int bs_id, char* bs_mac, g_network_para* g_network);
void send_ready_handover_signal(int bs_id, char* bs_mac, int quility, g_network_para* g_network);
void send_initcompleted_signal(int bs_id, g_network_para* g_network);

void send_linkclosed_signal(int bs_id, g_network_para* g_network);
void send_linkopen_signal(int bs_id, g_network_para* g_network);





#endif//BS_NETWORK_JSON_H











