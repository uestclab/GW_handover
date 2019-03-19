#ifndef GW_FRAME_H
#define GW_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#define BEACON                  0
#define ASSOCIATION_REQUEST     1
#define ASSOCIATION_RESPONSE    2
#define REASSOCIATION           3
#define DEASSOCIATION           4
#define HANDOVER_START_REQUEST  5
#define HANDOVER_START_RESPONSE 6


typedef struct management_frame_Info{
	int32_t type;
	int32_t subtype;
    int32_t length;              // mac_header(24) + payload
    char source_mac_addr[6];    // 6
    char dest_mac_addr[6];      // 6
	char Next_dest_mac_addr[6]; // 6
    char* payload;               //  payload cjson defined
}management_frame_Info;


int ManagementFrame_create_monitor_interface();
/*
  input  :
	time_cnt : 
		0: only send once without response
		!0 : wait for getting respose , configure time_cnt as timeout
  return : 
	0: success , frame_Info updated by new response management frame
	1: success , tx without response 
	2: failed
	3: timeout, No response management frame
*/
int handle_monitor_tx_with_response(management_frame_Info* frame_Info, int time_cnt);

/*
	input :
		time_cnt : poll cnt
	return :
		3: timeout
		0: success
*/
int gw_monitor_poll(management_frame_Info* frame_Info, int time_cnt);

management_frame_Info* new_air_frame(int32_t subtype, int32_t payload_len);

void close_monitor_interface();


#ifdef __cplusplus
}
#endif

#endif//GW_FRAME_H
