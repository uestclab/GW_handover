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


void hexdump(const void* p, size_t size) {
	const uint8_t *c = p;
	assert(p);

	printf("Dumping %u bytes from %p:\n", (unsigned int)size, p);

	while (size > 0) {
		unsigned i;

		for (i = 0; i < 16; i++) {
			if (i < size)
				printf("%02x ", c[i]);
			else
				printf("  ");
		}

		for (i = 0; i < 16; i++) {
			if (i < size)
				printf("%c", c[i] >= 32 && c[i] < 127 ? c[i] : '.');
			else
				printf(" ");
		}

		printf("\n");

		c += 16;

		if (size <= 16)
		break;

		size -= 16;
	}
} 


int get_mac(char * mac, int len_limit, char *arg)
{
    struct ifreq ifreq;
    int sock;

    if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror ("socket");
        return -1;
    }
    strcpy (ifreq.ifr_name, arg);

    if (ioctl (sock, SIOCGIFHWADDR, &ifreq) < 0)
    {
        perror ("ioctl");
        return -1;
    }

	close(sock);
    
    return snprintf (mac, len_limit, "%02x%02x%02x%02x%02x%02x", (unsigned char) ifreq.ifr_hwaddr.sa_data[0], (unsigned char) ifreq.ifr_hwaddr.sa_data[1], (unsigned char) ifreq.ifr_hwaddr.sa_data[2], (unsigned char) ifreq.ifr_hwaddr.sa_data[3], (unsigned char) ifreq.ifr_hwaddr.sa_data[4], (unsigned char) ifreq.ifr_hwaddr.sa_data[5]);
}

// in_addr[12] , out_addr[6] : 000c29d46f68 , 0
void change_mac_buf(char* in_addr, char* out_addr){
	int i = 0;
	char number = 0;
	char temp = 0;
	int cnt = 0;	
	for(i=0;i<12;i++){
		if(in_addr[i]>='0'&&in_addr[i]<='9')
			temp = in_addr[i] - '0';
		else if(in_addr[i]>='a'&&in_addr[i]<='f')
			temp = in_addr[i] - 'a' + 10;
		else if(in_addr[i]>='A'&&in_addr[i]<='F')
			temp = in_addr[i] - 'A' + 10;
		
		if(i%2 == 1){
			number = number * 16 + temp;
			out_addr[cnt] = number;
			number = 0;
			cnt = cnt + 1;
		}else{
			number = number + temp;
		}
	}
}

void reverseBuf(char* in_buf, char* out_buf, int number){
	int i,j;
	for(i=0,j=number-1;i<number;i++){
		out_buf[i] = in_buf[j];
		j=j-1;
	}
}

void fill_buffer(management_frame_Info* frame_Info, char* buf);
void parse_buffer(management_frame_Info* frame_Info , char* buf);


/*
#define BEACON                  0
#define ASSOCIATION_REQUEST     1
#define ASSOCIATION_RESPONSE    2
#define REASSOCIATION           3
#define DEASSOCIATION           4
#define HANDOVER_START_REQUEST  5
#define HANDOVER_START_RESPONSE 6
*/

int main(int argc, char *argv[])
{
	printf("argc = %d \n",argc);
	if(argc!=2)
	{
		printf("Usage : ethname\n");
		return 1;
	}
	
	char buf[1500];
	char mac_buf[6];
	char mac_buf_1[6];
	char mac_buf_2[6];

	char* mac_1 = "10ec2bd46c58";
	char* mac_2 = "ffffffffddd0";

    char    szMac[18];
    int        nRtn = get_mac(szMac, sizeof(szMac),argv[1]);
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
	
	change_mac_buf(mac_1,mac_buf_1);
	hexdump(mac_buf_1,6);

	change_mac_buf(mac_2,mac_buf_2);
	hexdump(mac_buf_2,6);
	
// ------------- send frame ------------------------	
	char* json = "Hello managetment frame!";	
	int json_length = strlen(json) + 1;

	management_frame_Info* temp_Info = (management_frame_Info*)malloc(sizeof(management_frame_Info));
	temp_Info->type = 0;
	temp_Info->subtype = HANDOVER_START_RESPONSE; ///---------subtype
	temp_Info->length  = 24 + 2 + json_length;
	memcpy(temp_Info->source_mac_addr,mac_buf,6);
	memcpy(temp_Info->dest_mac_addr,mac_buf_1,6);
	memcpy(temp_Info->Next_dest_mac_addr,mac_buf_2,6);
	
	if(temp_Info->length > 24 + 2){
		temp_Info->payload = malloc(json_length);
		memcpy(temp_Info->payload,json,json_length);
	}

	printf("--- send : type = %d , subtype = %d ,length = %d \n" , temp_Info->type, temp_Info->subtype, temp_Info->length);

	fill_buffer(temp_Info,buf);
	hexdump(buf,temp_Info->length);
	printf("buf str = %d \n",temp_Info->length); // error

	printf("------------------- parse -------------------\n");


	char* json_tmp = "other frame!";	
	int json_length_tmp = strlen(json_tmp) + 1;

	management_frame_Info* rece_Info = (management_frame_Info*)malloc(sizeof(management_frame_Info));
	rece_Info->type = 0;
	rece_Info->subtype = 5;
	rece_Info->length  = 26 + json_length_tmp;
	
	if(temp_Info->length > 26){
		rece_Info->payload = malloc(json_length_tmp);
		memcpy(rece_Info->payload,json_tmp,json_length_tmp);
	}

	parse_buffer(rece_Info , buf);
	printf("--- receive : subtype = %d, length = %d \n", rece_Info->subtype , rece_Info->length);
	if(rece_Info->length > 26)
		printf(" receive payload = %s \n",rece_Info->payload);
	printf("\nreceive source_mac_addr: \n");
	hexdump(rece_Info->source_mac_addr,6);
	printf("\nreceive dest_mac_addr: \n");
	hexdump(rece_Info->dest_mac_addr,6);
	printf("\nreceive Next_dest_mac_addr: \n");
	hexdump(rece_Info->Next_dest_mac_addr,6);
	
    return 0;
}


/*
	hexdump(&frame_control,4);
	char reverse_buf[4];
	char in_buf[4];
	memcpy(in_buf,&frame_control,4);
	reverseBuf(in_buf,reverse_buf,4);
	printf("----- inbuf = %d , reverse_buf = %d \n", *((unsigned int*)(in_buf)),*((unsigned int*)(reverse_buf)));
	memcpy(buf,reverse_buf,4);
*/




