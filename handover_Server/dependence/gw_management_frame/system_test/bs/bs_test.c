#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>        //for struct ifreq

#include "gw_frame.h"
#include "gw_utility.h"


/*
#define BEACON                  0
#define ASSOCIATION_REQUEST     1
#define ASSOCIATION_RESPONSE    2
#define REASSOCIATION           3
#define DEASSOCIATION           4
#define HANDOVER_START_REQUEST  5
#define HANDOVER_START_RESPONSE 6
*/

char mac_buf[6];
char mac_buf_dest[6];
char mac_buf_next[6];


void testCase_beacon(){
	printf("\n------------- start test beacon process ------------------------\n");
	/* 
		check if receive beacon frame (if yes)
		1. get source_mac_addr
		2. send ASSOCIATION_REQUEST frame
		3. 
	*/
	
	int receive_beacon = 0;
	char ve_mac_buf[6];	
	
	management_frame_Info* temp_Info = new_air_frame(-1, 0);
	int time_cnt = 10 * 200;

	while(receive_beacon == 0){
		int stat = gw_monitor_poll(temp_Info, time_cnt);

		if(stat == 0){
			printf("receive new air frame \n");
			if(temp_Info->subtype == BEACON){
				receive_beacon = 1;
				memcpy(ve_mac_buf,temp_Info->source_mac_addr,6);
				if(temp_Info->payload != NULL){
					free(temp_Info->payload);
					free(temp_Info);
				}
				break;
			}
		}else if(stat == 3){
			printf("timeout\n");
		}
	}
	
	// after receive beacon frame

	
	management_frame_Info* send_Info = new_air_frame(ASSOCIATION_REQUEST, 0); // ASSOCIATION_REQUEST
	memcpy(send_Info->source_mac_addr,mac_buf,6);
	memcpy(send_Info->dest_mac_addr,ve_mac_buf,6);
	memcpy(send_Info->Next_dest_mac_addr,mac_buf_next,6);

	printf("--- send : type = %d , subtype = %d ,length = %d \n" , send_Info->type, send_Info->subtype, send_Info->length);
	
	int status = 3;
	time_cnt = 3;
	while(1){
		status = handle_monitor_tx_with_response(send_Info, time_cnt);
		printf("---------------status = %d \n", status);
		if(status == 0){
			printf("receive response frame \n");
			if(send_Info->subtype == ASSOCIATION_RESPONSE){
				printf("receive ASSOCIATION_RESPONSE frame\n");
				break;
			}
		}
	}
	
	free(send_Info);

	printf("\n------------- completed test beacon process ------------------------\n");
}



int main(int argc, char *argv[])
{
	printf("\n###############  baseStation start test air signaling  ###############\n");

	/* 
		test case  (no state variable control) (re_transmit control by self)
	*/
	// get mac by self


// ------------- vehicle info init ------------------------
	printf("argc = %d \n",argc);
	if(argc!=2)
	{
		printf("Usage : ethname\n");
		return 1;
	}

	char* mac_1 = "10ec2bd46c58";
	char* mac_2 = "ffffffffddd0";

    char    szMac[18];
    int     nRtn = get_mac(szMac, sizeof(szMac),argv[1]);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("argv[1] = %s , nRtn = %d , MAC ADDR : %s\n", argv[1],nRtn,szMac);
		printf("mac_1 : %s \n",mac_1);
		printf("mac_2 : %s \n",mac_2);
    }else{
		return 0;
	}
	
	change_mac_buf(szMac,mac_buf);
	hexdump(mac_buf,6);
	
	change_mac_buf(mac_1,mac_buf_dest);
	hexdump(mac_buf_dest,6);

	change_mac_buf(mac_2,mac_buf_next);
	hexdump(mac_buf_next,6);

	ManagementFrame_create_monitor_interface();
	
	// receive beacon frame , then send ASSOCIATION_REQUEST frame
	testCase_beacon();
	

    return 0;
}









