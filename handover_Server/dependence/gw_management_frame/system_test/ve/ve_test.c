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
#include "gw_control.h"

#include "zlog.h"

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

char mac_buf[6];
char mac_buf_dest[6];
char mac_buf_next[6];

// ve

void testCase(zlog_category_t *zlog_handler){
	zlog_info(zlog_handler,"\n------------- start test case ------------------------\n");

	uint16_t tx_seq_id = 0;
	uint16_t expect_seq_id = 0;	

	int time_cnt;
	int status;
	while(1){
		management_frame_Info* beacon_Info = new_air_frame(BEACON, 0,mac_buf,mac_buf_dest,mac_buf_next,tx_seq_id); // BEACON
		status = handle_air_tx(beacon_Info,zlog_handler);
		if(status == 0){
			zlog_info(zlog_handler,"send BEACON success : tx_seq_id = %d\n",tx_seq_id);
			time_cnt = 3;
			int stat = gw_monitor_poll(beacon_Info, time_cnt, zlog_handler); // 3 * 5ms
			if(stat == 26){
				zlog_info(zlog_handler,"--- receive : subtype = %d, length = %d \n", beacon_Info->subtype , beacon_Info->length);
				if(beacon_Info->subtype == ASSOCIATION_REQUEST){
					zlog_info(zlog_handler,"--- receive : ASSOCIATION_REQUEST : id = %d \n",beacon_Info->seq_id);
					if(expect_seq_id != beacon_Info->seq_id){
						zlog_info(zlog_handler,"received id != expect id : %d , %d --------------\n", beacon_Info->seq_id,expect_seq_id);
						printf("received id != expect id : %d , %d --------------\n", beacon_Info->seq_id,expect_seq_id);
						user_wait();
					}
					expect_seq_id = expect_seq_id + 1;
				}else{
					zlog_info(zlog_handler,"!!!!!!!! received other type : %d \n",beacon_Info->subtype);
				}
			}else{
				zlog_info(zlog_handler,"poll timeout,status = %d \n" , stat);			
			}
		}else if(status < 0){
			zlog_info(zlog_handler,"air_tx,status = %d \n" , status);
		}
		tx_seq_id = tx_seq_id + 1;
		free(beacon_Info);
	}

	zlog_info(zlog_handler,"\n------------- test case completed ------------------------\n");

}

zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;

	rc = zlog_init(path);

	if (rc) {
		printf("init serverLog failed\n");
		return NULL;
	}

	zlog_handler = zlog_get_category("ve_test_log");

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

//int main(int argc, char *argv[])
int main(int argc, char *argv[])
{

	zlog_category_t *zlog_handler = serverLog("/run/media/mmcblk1p1/etc/zlog_default.conf");
	zlog_info(zlog_handler,"\n###############  vehicle start test ###############\n");


	//char* mac_1 = "aabbccddeeff";
	//char* mac_2 = "112233445566";
	
	char* mac_1 = "000000000000";
	char* mac_2 = "000000000000";
	change_mac_buf(mac_1,mac_buf);	
	change_mac_buf(mac_1,mac_buf_dest);
	change_mac_buf(mac_2,mac_buf_next);

	int fd = ManagementFrame_create_monitor_interface();

	zlog_info(zlog_handler,"ManagementFrame_create_monitor_interface : fd = %d \n" , fd);
	zlog_info(zlog_handler,"mac_1 = %s \n" , mac_1);
	
	// send beacon frame
	testCase(zlog_handler);
	

    return 0;
}



