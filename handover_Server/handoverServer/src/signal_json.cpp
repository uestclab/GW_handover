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

void send_init_distance_signal(BaseStation* bs, int bs_id, int ve_id){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "init_distance_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddNumberToObject(root, "ve_id", ve_id);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::INIT_DISTANCE, json_buf);
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

void send_start_handover_signal(BaseStation* bs, BaseStation* next_bs, int bs_id, char* mac){

	char* remote_ip = next_bs->getBsIP();

	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "start_handover_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	cJSON_AddStringToObject(root, "target_bs_mac", mac);
	cJSON_AddStringToObject(root, "target_bs_ip", remote_ip);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::START_HANDOVER, json_buf);
	cJSON_Delete(root);
}

void send_change_tunnel_ack_signal(BaseStation* bs, int bs_id){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "change_tunnel_ack_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::CHANGE_TUNNEL_ACK,json_buf);
	cJSON_Delete(root);
}

void send_server_recall_monitor_signal(BaseStation* bs, int bs_id/*int punish_time */){
	cJSON* root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "signal", "server_recall_monitor_signal");
    cJSON_AddNumberToObject(root, "bs_id", bs_id);
	char* json_buf = cJSON_Print(root);
	bs->sendSignal(glory::SERVER_RECALL_MONITOR,json_buf);
	cJSON_Delete(root);
}









