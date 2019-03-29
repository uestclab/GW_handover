#include "bs_network_json.h"
#include "cJSON.h"

void send_id_pair_signal(int bs_id, char* bs_mac, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "id_pair_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
    cJSON_AddStringToObject(root, "bs_mac_addr", bs_mac);
	char* json_buf = cJSON_Print(root);
	sendSignal(ID_PAIR, json_buf, g_network);
	cJSON_Delete(root);
}

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

void send_initcompleted_signal(int bs_id, g_network_para* g_network){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "initcompleted_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	char* json_buf = cJSON_Print(root);
	sendSignal(INIT_COMPLETED, json_buf, g_network);
	cJSON_Delete(root);
}


