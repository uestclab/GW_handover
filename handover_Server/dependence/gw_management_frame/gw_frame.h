#ifndef GW_FRAME_H
#define GW_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "zlog.h"

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
	uint16_t seq_id;
}management_frame_Info;


int ManagementFrame_create_monitor_interface();

// new interface

/*
	air_tx : return : byte_size success, -1 input parameter error, < 0 tx fail
*/
int handle_air_tx(management_frame_Info* frame_Info, zlog_category_t *zlog_handler);
//

/*
	air_rx :
	input :
		time_cnt : poll cnt --> block time = cnt * 5ms
	return :
		>0: success
*/
int gw_monitor_poll(management_frame_Info* frame_Info, int time_cnt, zlog_category_t *zlog_handler);

management_frame_Info* new_air_frame(int32_t subtype, int32_t payload_len, 
								char* mac_buf, char* mac_buf_dest, char* mac_buf_next,uint16_t seq_id);

void close_monitor_interface();


#ifdef __cplusplus
}
#endif

#endif//GW_FRAME_H
