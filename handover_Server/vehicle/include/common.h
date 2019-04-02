#ifndef VEHICLE_COMMON_H
#define VEHICLE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct system_info_para{
	int32_t     ve_state; // indicate system state while time flow
	char        link_bs_mac[6];
	char        ve_mac[6];
	char        next_bs_mac[6];
	int         isLinked;    
}system_info_para;


typedef struct ConfigureNode{
	system_info_para*  system_info;
	int32_t vehicle_id;
	char* my_mac_str;
	char* my_Ethernet;
}ConfigureNode;



#endif//VEHICLE_COMMON_H
