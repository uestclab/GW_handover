#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>        //for struct ifreq
#include <sys/time.h>

#include "gw_frame.h"
#include "gw_utility.h"

#include "zlog.h"


/*
#define BEACON                  0
#define ASSOCIATION_REQUEST     1
#define ASSOCIATION_RESPONSE    2
#define REASSOCIATION           3
#define DEASSOCIATION           4
#define HANDOVER_START_REQUEST  5
#define HANDOVER_START_RESPONSE 6
*/
void user_wait()
{
	int c;
	printf("user_wait... \n");
	do
	{
		c = getchar();
		if(c == 'g') break;
	} while(c != '\n');
}
/*
	int64_t start = now();
	.....
	int64_t end = now();
	double sec = (end-start)/1000000.0;
	printf("%f sec %f ms \n", sec, 1000*sec);
*/
int64_t now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

char mac_buf[6];
char mac_buf_dest[6];
char mac_buf_next[6];


// bs

void testCase(zlog_category_t *zlog_handler){
	
	char ve_mac_buf[6];
	uint16_t tx_seq_id = 0;
	uint16_t expect_seq_id = 0;	
	
	int time_cnt = 1000;
	int status;
	while(1){ // wait for beacon
		management_frame_Info* temp_Info = new_air_frame(-1,0,NULL,NULL,NULL,0);
		int64_t start = now();
		int stat = gw_monitor_poll(temp_Info, time_cnt,zlog_handler); // receive 1000 * 5ms

		if(stat == 26){
			//zlog_info(zlog_handler,"receive new air frame \n");
			if(temp_Info->subtype == BEACON){
				zlog_info(zlog_handler,"receive: BEACON id = %d \n" , temp_Info->seq_id);
				memcpy(ve_mac_buf,temp_Info->source_mac_addr,6);

				if(expect_seq_id != temp_Info->seq_id){
					printf("received id != expect id : %d , %d --------\n",temp_Info->seq_id,expect_seq_id);
					expect_seq_id = temp_Info->seq_id;					
					user_wait();				
				}
				expect_seq_id = expect_seq_id + 1;
				int64_t end = now();
				double sec = (end-start)/1000000.0;
				zlog_info(zlog_handler,"---------- %f sec %f ms \n", sec, 1000*sec);
			}else{
				zlog_info(zlog_handler,"!!!!!!!! received other type : %d \n",temp_Info->subtype);
			}
		}else if(stat <= 0){
			zlog_info(zlog_handler,"poll timeout , stat = %d !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" , stat);
		}
		free(temp_Info);
	}

	zlog_info(zlog_handler,"\n------------- completed test beacon process ------------------------\n");
}


zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;

	rc = zlog_init(path);

	if (rc) {
		printf("init serverLog failed\n");
		return NULL;
	}

	zlog_handler = zlog_get_category("bs_test_log");

	if (!zlog_handler) {
		printf("get cat fail\n");

		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeServerLog(){
	zlog_fini();
}

int main(int argc, char *argv[])
{
	zlog_category_t *zlog_handler = serverLog("/run/media/mmcblk1p1/etc/zlog_default.conf");

	zlog_info(zlog_handler,"\n###############  baseStation start test ###############\n");

	char* mac_1 = "000000000000";
	char* mac_2 = "000000000000";

	change_mac_buf(mac_1,mac_buf);	
	change_mac_buf(mac_1,mac_buf_dest);
	change_mac_buf(mac_2,mac_buf_next);

	int fd = ManagementFrame_create_monitor_interface();
	zlog_info(zlog_handler,"ManagementFrame_create_monitor_interface : fd = %d \n" , fd);
	// test
	testCase(zlog_handler);
	

    return 0;
}









