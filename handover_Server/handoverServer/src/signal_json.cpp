#include "signal_json.h"
#include "common.h"

void send_id_received_signal(BaseStation* bs, int bs_id){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "id_received_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::ID_RECEIVED,json_buf);
	cJSON_Delete(root);
}

void send_init_location_signal(BaseStation* bs, int bs_id){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "init_location_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::INIT_LOCATION, json_buf);
	cJSON_Delete(root);
}

void send_init_link_signal(BaseStation* bs, int bs_id, char* mac){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "init_link_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddStringToObject(root, "bs_mac", mac);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::INIT_LINK, json_buf);
	cJSON_Delete(root);
}

void send_start_handover_signal(BaseStation* bs, int bs_id, char* mac){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "start_handover_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddStringToObject(root, "target_bs_mac", mac);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::START_HANDOVER, json_buf);
	cJSON_Delete(root);
}








