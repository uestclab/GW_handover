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


void testCase_1(){
	printf("\n------------- start test case_1 ------------------------\n");
	// session 1: send beacon frame periodic
	int received_request = 0;
	char bs_mac_buf[6];

	char* ve_mac_json = "11eeffccdd50";	
	int json_length = strlen(ve_mac_json) + 1;

	management_frame_Info* temp_Info = new_air_frame(BEACON, json_length); // BEACON
	memcpy(temp_Info->source_mac_addr,mac_buf,6);
	memcpy(temp_Info->dest_mac_addr,mac_buf_dest,6);
	memcpy(temp_Info->Next_dest_mac_addr,mac_buf_next,6);
	memcpy(temp_Info->payload,ve_mac_json,json_length);

	printf("--- send : type = %d , subtype = %d ,length = %d \n" , temp_Info->type, temp_Info->subtype, temp_Info->length);

	int i;
	int time_cnt;
	int status;
	int retransmit = 0; // test number
	while(received_request == 0){
		time_cnt = 3;
		status = handle_monitor_tx_with_response(temp_Info, time_cnt); // 3 * 5ms
		printf("---------------status = %d \n", status);
		if(status == 0){
			printf("\n------------------- receive new message and parse -------------------\n");
			printf("--- receive : subtype = %d, length = %d \n", temp_Info->subtype , temp_Info->length);
			if(temp_Info->length > 26){
				printf(" receive payload = %s \n",temp_Info->payload);
			}
			printf("\nreceive source_mac_addr: \n");
			hexdump(temp_Info->source_mac_addr,6);
			printf("\nreceive dest_mac_addr: \n");
			hexdump(temp_Info->dest_mac_addr,6);
			printf("\nreceive Next_dest_mac_addr: \n");
			hexdump(temp_Info->Next_dest_mac_addr,6);
			if(temp_Info->subtype == ASSOCIATION_REQUEST){
				printf("receive ASSOCIATION_REQUEST from baseStation, retransmit = %d \n" ,retransmit);
				retransmit = 0;
				received_request = 1;
				memcpy(bs_mac_buf,temp_Info->source_mac_addr,6);
				break;
			}
		}
	}
	free(temp_Info->payload);
	free(temp_Info);

	// session 2: 
	if(received_request == 0){
		printf("end testCase_1()\n");
		return;
	}
	
	while(1){
		management_frame_Info* response_frame = new_air_frame(ASSOCIATION_RESPONSE, 0);
		memcpy(response_frame->source_mac_addr,mac_buf,6);
		memcpy(response_frame->dest_mac_addr,bs_mac_buf,6);
		memcpy(response_frame->Next_dest_mac_addr,mac_buf_next,6);
		status = handle_monitor_tx_with_response(response_frame, 0);
		printf("---------------status = %d \n", status);
		if(status == 1){
			free(response_frame);
		}

		// check poll interface

	}
	

	printf("\n------------- test case_1 completed ------------------------\n");

}



int main(int argc, char *argv[])
{
	printf("\n###############  vehicle start test air signaling  ###############\n");

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

	char* mac_1 = "ffffffffffff";
	char* mac_2 = "0922ddaa77d0";

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
	
	// send beacon frame
	testCase_1();
	

    return 0;
}




