#include "bs_network_json.h"
#include "cJSON.h"

//ID_PAIR
void send_id_pair_signal(int bs_id, char* bs_mac, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "id_pair_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
    cJSON_AddStringToObject(root, "bs_mac_addr", bs_mac);
	char* json_buf = cJSON_Print(root);
	sendSignal(ID_PAIR, json_buf, g_network);
	cJSON_Delete(root);
}

// inform server when received beacon
void send_recvbeacon_signal(int bs_id, int ve_id, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "recvbeacon_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "ve_id", ve_id);
	char* json_buf = cJSON_Print(root);
	sendSignal(BEACON_RECV, json_buf, g_network);
	cJSON_Delete(root);
}

// bs get distance and inform server
void send_init_dist_over_signal(int bs_id, int ve_id, double dist, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "init_dist_over_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "ve_id", ve_id);
	cJSON_AddNumberToObject(root, "dist", dist);
	char* json_buf = cJSON_Print(root);
	sendSignal(INIT_DISTANCE_OVER, json_buf, g_network);
	cJSON_Delete(root);
}

//READY_HANDOVER
void send_ready_handover_signal(int bs_id, char* bs_mac, int quility, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "ready_handover_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "quility", quility);
    cJSON_AddStringToObject(root, "bs_mac_addr", bs_mac);
	char* json_buf = cJSON_Print(root);
	sendSignal(READY_HANDOVER, json_buf, g_network);
	cJSON_Delete(root);
}

//INIT_COMPLETED
void send_initcompleted_signal(int bs_id, int ve_id, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "initcompleted_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "ve_id", ve_id);
	char* json_buf = cJSON_Print(root);
	sendSignal(INIT_COMPLETED, json_buf, g_network);
	cJSON_Delete(root);
}

//LINK_CLOSED
void send_linkclosed_signal(int bs_id, int ve_id, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "link_closed_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "ve_id", ve_id);
	char* json_buf = cJSON_Print(root);
	sendSignal(LINK_CLOSED, json_buf, g_network);
	cJSON_Delete(root);
}

//LINK_OPEN
void send_linkopen_signal(int bs_id, int ve_id, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "link_open_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "ve_id", ve_id);
	char* json_buf = cJSON_Print(root);
	sendSignal(LINK_OPEN, json_buf, g_network);
	cJSON_Delete(root);
}

//CHANGE_TUNNEL
void send_change_tunnel_signal(int bs_id, int ve_id, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "change_tunnel_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "ve_id", ve_id);
	char* json_buf = cJSON_Print(root);
	sendSignal(CHANGE_TUNNEL, json_buf, g_network);
	cJSON_Delete(root);
}

void send_dac_closed_x2_signal(int bs_id, char* my_ip, g_x2_para* g_x2){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "x2signal", "dac_closed");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddStringToObject(root, "own_ip", my_ip);
	char* json_buf = cJSON_Print(root);
	sendX2Signal(json_buf, g_x2);
	cJSON_Delete(root);
	free(json_buf);
}

void send_dac_closed_x2_ack_signal(int bs_id, g_x2_para* g_x2){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "x2signal", "dac_closed_ack");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	char* json_buf = cJSON_Print(root);
	sendX2Signal(json_buf, g_x2);
	cJSON_Delete(root);
	free(json_buf);
}


