#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cJSON.h"
#include "gw_utility.h"
#include "gw_control.h"
#include "broker.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>


char *get_prog_name(char *argv)
{
	int len = strlen(argv);
	int i;
	char *tmp = argv;
	
	for(i=len; i >=0; i--)
	{
		if(tmp[i] == '/'){
			i++;
			break;
		}
	}
	
	if(-1 == i){
		i = 0;
	}

	return argv + i;
}

int initBroker(char *argv, recv_cb exception_cb){

	int ret = init_broker(get_prog_name(argv), NULL, -1, NULL, NULL);
	printf("get_prog_name(argv) = %s , ret = %d \n",get_prog_name(argv),ret);
	if( ret != 0)
		return -2;

	ret = register_callback("all", exception_cb, "#");
	if(ret != 0){
		printf("register_callback error in initBroker\n");
		return -1;
	}

	return 0;
}

 /* ----------------------- new interface to read and write register ---------------------------- */

int initRegdev(g_RegDev_para** g_RegDev, zlog_category_t* handler)
{
	zlog_info(handler,"initPhyRegdev()");

	*g_RegDev = (g_RegDev_para*)malloc(sizeof(struct g_RegDev_para));

	(*g_RegDev)->mem_dev_phy = NULL;
	(*g_RegDev)->mem_dev_c1  = NULL;
	(*g_RegDev)->mem_dev_c0  = NULL;
	(*g_RegDev)->dac_fd      = -1;
	(*g_RegDev)->log_handler = handler;


	int rc = 0;

	regdev_init(&((*g_RegDev)->mem_dev_phy));
	regdev_set_para((*g_RegDev)->mem_dev_phy, REG_PHY_ADDR, REG_MAP_SIZE);
	rc = regdev_open((*g_RegDev)->mem_dev_phy);
	if(rc < 0){
		zlog_info(handler," mem_dev_phy regdev_open err !!\n");
		return -1;
	}

// mem_dev_c1
	regdev_init(&((*g_RegDev)->mem_dev_c1));
	regdev_set_para((*g_RegDev)->mem_dev_c1, REG_C1_ADDR, REG_MAP_SIZE);
	rc = regdev_open((*g_RegDev)->mem_dev_c1);
	if(rc < 0){
		zlog_info(handler," mem_dev_c1 regdev_open err !!\n");
		return -1;
	}

// mem_dev_c0
	regdev_init(&((*g_RegDev)->mem_dev_c0));
	regdev_set_para((*g_RegDev)->mem_dev_c0, REG_C0_ADDR, REG_MAP_SIZE);
	rc = regdev_open((*g_RegDev)->mem_dev_c0);
	if(rc < 0){
		zlog_info(handler," mem_dev_c0 regdev_open err !!\n");
		return -1;
	}

	(*g_RegDev)->dac_fd=open(SYSFS_GPIO_RST_VAL,O_RDWR);
	if((*g_RegDev)->dac_fd == -1){
		zlog_info(handler," dac fd open err !!\n");
		return -1;
	}

	return 0;
}


int disable_dac(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"disable_dac\n");
	int rc = 0;
	rc = write(g_RegDev->dac_fd, SYSFS_GPIO_RST_VAL_L, sizeof(SYSFS_GPIO_RST_VAL_L));
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"error in disable_dac , rc = %d \n", rc);
		return rc;
	}

	return rc;
}

int enable_dac(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"enable_dac\n");
	int rc = 0;
	rc = write(g_RegDev->dac_fd, SYSFS_GPIO_RST_VAL_H, sizeof(SYSFS_GPIO_RST_VAL_H));
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"error in enable_dac , rc = %d \n", rc);
		return rc;
	}
	return rc;
}

int set_dst_mac_fast(g_RegDev_para* g_RegDev, char* dst_mac_buf){ // mac_buf[6]
	zlog_info(g_RegDev->log_handler,"set_dst_mac_fast\n");

	uint32_t low32 = getLow32(dst_mac_buf);
	uint32_t high16 = getHigh16(dst_mac_buf);	
	
	// dest_mac_0_low_32_bit
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x838, low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"dest_mac_0_low_32_bit write failed !!! \n");
		return rc;
	}
	//dest_mac_1_high_16_bit
	rc = regdev_write(g_RegDev->mem_dev_phy, 0x83c, high16);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"dest_mac_1_high_16_bit write failed !!! \n");
		return rc;
	}
	return 0;
}

int set_src_mac_fast(g_RegDev_para* g_RegDev, char* src_mac_buf){ //src_mac_buf[6]
	zlog_info(g_RegDev->log_handler,"set_src_mac_fast\n");

	uint32_t low32 = getLow32(src_mac_buf);
	uint32_t high16 = getHigh16(src_mac_buf);

	// src_mac_0_low_32_bit
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x830, low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"src_mac_0_low_32_bit write failed !!! \n");
		return rc;
	}
	//src_mac_1_high_16_bit
	rc = regdev_write(g_RegDev->mem_dev_phy, 0x834, high16);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"src_mac_1_high_16_bit write failed !!! \n");
		return rc;
	}
	return 0;
}

/* ----------------------------- process ddr_tx_hold_on and mac_id sync ---------------------------- */
 
// bit : 2 , 0 -- open , 1 -- close
int open_ddr(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"open_ddr\n");
	uint32_t value = 0x00000000;
	//value = value << 24;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x82c, value); // Note :
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"open_ddr write failed !!! \n");
		return rc;
	}
	return 0;
}

int close_ddr(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"close_ddr\n");
	uint32_t value = 0x00000004;
	//value = value << 24;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x82c, value); // Note :
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"close_ddr write failed !!! \n");
		return rc;
	}
	return 0;
}

// bit : 1 , 1 -- trigger , fpga set 0 
int trigger_mac_id(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"trigger_mac_id\n");
	uint32_t value = 0x00000002;
	//value = value << 24;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x82c, value); // Note :
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"trigger_mac_id write failed !!! \n");
		return rc;
	}
	return 0;
}

/* ----------------------------- get ddr_full_flag , airdata_buf2_empty_flag , airsignal_buf2_empty_flag ---------------------------- */



uint32_t ddr_empty_flag(g_RegDev_para* g_RegDev){ // value != 0 : full
	uint32_t flag = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x82c, &flag);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"ddr_empty_flag failed !!! \n");
		return rc;
	}

	// bit3 ~ bit4
	uint32_t value = flag & (0x3<<3);
	value = value>>3;

	return value;
}

uint32_t airdata_buf2_empty_flag(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"airdata_buf2_empty_flag\n");
	uint32_t fifo_data_cnt_read = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x81c, &fifo_data_cnt_read);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"fifo_data_cnt_read failed !!! \n");
		return rc;
	}

	// bit0 ~ bit4
	uint32_t fifo_data_cnt = fifo_data_cnt_read & (0x1f);

	zlog_info(g_RegDev->log_handler,"fifo_data_cnt = %u \n",fifo_data_cnt);

// ------------------------------------------------------------

	uint32_t tx_win_cnt_read = 0x00000000;
	int	rc_1 = regdev_read(g_RegDev->mem_dev_phy, 0x80c, &tx_win_cnt_read);
	if(rc_1 < 0){
		zlog_info(g_RegDev->log_handler,"tx_win_cnt_read failed !!! \n");
		return rc_1;
	}

	// bit8 ~ bit10
	uint32_t tx_win_cnt = tx_win_cnt_read & (0x7<<8);
	tx_win_cnt = tx_win_cnt>>8;

	zlog_info(g_RegDev->log_handler,"tx_win_cnt = %u \n",tx_win_cnt);

	if(fifo_data_cnt == 0 && tx_win_cnt == 0)
		return 1;
	else
		return 0;
}

uint32_t airsignal_buf2_empty_flag(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"airsignal_buf2_empty_flag\n");
	uint32_t sw_fifo_data_cnt_read = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x81c, &sw_fifo_data_cnt_read);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"sw_fifo_data_cnt_read failed !!! \n");
		return rc;
	}

	// bit7 ~ bit9
	uint32_t sw_fifo_data_cnt = sw_fifo_data_cnt_read & (0x7<<7);
	sw_fifo_data_cnt = sw_fifo_data_cnt>>7;

	zlog_info(g_RegDev->log_handler,"sw_fifo_data_cnt = %u \n",sw_fifo_data_cnt);

// ------------------------------------------------------------

	uint32_t sw_tx_win_cnt_read = 0x00000000;
	int	rc_1 = regdev_read(g_RegDev->mem_dev_phy, 0x80c, &sw_tx_win_cnt_read);
	if(rc_1 < 0){
		zlog_info(g_RegDev->log_handler,"sw_tx_win_cnt_read failed !!! \n");
		return rc_1;
	}

	// bit20 ~ bit21
	uint32_t sw_tx_win_cnt = sw_tx_win_cnt_read & (0x3<<20);
	sw_tx_win_cnt = sw_tx_win_cnt>>20;

	zlog_info(g_RegDev->log_handler,"sw_tx_win_cnt = %u \n",sw_tx_win_cnt);

	if(sw_fifo_data_cnt == 0 && sw_tx_win_cnt == 0)
		return 1;
	else
		return 0;
}



/* ----------------------------- get power , crc correct cnt , crc error cnt ---------------------------- */

uint32_t getPowerLatch(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"getPowerLatch\n");
	uint32_t power = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x124, &power);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"getPowerLatch failed !!! \n");
		return rc;
	}
	return power;
}

uint32_t get_crc_correct_cnt(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"get_crc_correct_cnt\n");
	uint32_t crc_correct_cnt = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x844, &crc_correct_cnt);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_crc_correct_cnt failed !!! \n");
		return rc;
	}
	return crc_correct_cnt;
}

uint32_t get_crc_error_cnt(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"get_crc_error_cnt\n");
	uint32_t crc_error_cnt = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_phy, 0x848, &crc_error_cnt);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"get_crc_error_cnt failed !!! \n");
		return rc;
	}
	return crc_error_cnt;
}


// ----------------------------------------------------------------------------------------

// read_unfilter_byte : 0x43c00290 0x43c00294 
uint32_t read_unfilter_byte_low32(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"read_unfilter_byte_low32\n");
	uint32_t low32 = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_c0, 0x290, &low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"read_unfilter_byte_low32 failed !!! \n");
		return rc;
	}
	return low32;
}

uint32_t read_unfilter_byte_high32(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"read_unfilter_byte_high32\n");
	uint32_t high32 = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_c0, 0x294, &high32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"read_unfilter_byte_high32 failed !!! \n");
		return rc;
	}
	return high32;
}


// rx_byte_filter_ether : 0x43c10460  0x43c10464  
uint32_t rx_byte_filter_ether_low32(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"rx_byte_filter_ether_low32\n");
	uint32_t low32 = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_c1, 0x460, &low32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"rx_byte_filter_ether_low32 failed !!! \n");
		return rc;
	}
	return low32;
}

uint32_t rx_byte_filter_ether_high32(g_RegDev_para* g_RegDev){
	//zlog_info(g_RegDev->log_handler,"rx_byte_filter_ether_high32\n");
	uint32_t high32 = 0x00000000;
	int	rc = regdev_read(g_RegDev->mem_dev_c0, 0x464, &high32);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"rx_byte_filter_ether_high32 failed !!! \n");
		return rc;
	}
	return high32;
}


// reset bb
//devmem 0x43c20004  32 0x2 \n
//devmem 0x43c20004  32 0x0 \n
int reset_bb(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"reset_bb\n");
	uint32_t value = 0x00000002;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x4, value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"reset_bb_1 write failed !!! \n");
		return rc;
	}

	return 0;
}

int release_bb(g_RegDev_para* g_RegDev){
	zlog_info(g_RegDev->log_handler,"reset_bb\n");
	uint32_t value = 0x0;
	int	rc = regdev_write(g_RegDev->mem_dev_phy, 0x4, value);
	if(rc < 0){
		zlog_info(g_RegDev->log_handler,"reset_bb_1 write failed !!! \n");
		return rc;
	}
	return 0;
}








