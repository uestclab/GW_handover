#ifndef GW_CONTROL_H
#define GW_CONTROL_H

#include "zlog.h"
#include <stdint.h>

#define	REG_PHY_ADDR	0x43C20000
#define	REG_MAP_SIZE	0X10000

#define SYSFS_GPIO_EXPORT           "/sys/class/gpio/export"
#define SYSFS_GPIO_RST_PIN_VAL      "48"
#define SYSFS_GPIO_RST_DIR          "/sys/class/gpio/gpio973/direction"
#define SYSFS_GPIO_RST_DIR_VAL      "OUT"
#define SYSFS_GPIO_RST_VAL          "/sys/class/gpio/gpio973/value"
#define SYSFS_GPIO_RST_VAL_H        "1"
#define SYSFS_GPIO_RST_VAL_L        "0"

typedef int (*recv_cb)(char* buf, int buf_len, char *from, void* arg);

int initBroker(char *argv, recv_cb exception_cb);

int set_dst_mac(char* low_32_str, char* high_16_str);
int set_src_mac(char* low_32_str, char* high_16_str);

typedef struct g_RegDev_para{
	void*  			   mem_dev_phy;
	int                dac_fd;
	zlog_category_t*   log_handler;
}g_RegDev_para;

 /* ----------------------- new interface to read and write register ---------------------------- */
int initRegdev(g_RegDev_para** g_RegDev, zlog_category_t* handler);
int disable_dac(g_RegDev_para* g_RegDev);
int enable_dac(g_RegDev_para* g_RegDev);
int set_dst_mac_fast(g_RegDev_para* g_RegDev, char* dst_mac_buf);
int set_src_mac_fast(g_RegDev_para* g_RegDev, char* src_mac_buf);

/* ----------------------------- process ddr_tx_hold_on and mac_id sync ---------------------------- */
int open_ddr(g_RegDev_para* g_RegDev);
int close_ddr(g_RegDev_para* g_RegDev);
int trigger_mac_id(g_RegDev_para* g_RegDev);

/* ----------------------------- get power , crc correct cnt , crc error cnt ---------------------------- */
uint32_t getPowerLatch(g_RegDev_para* g_RegDev);
uint32_t get_crc_correct_cnt(g_RegDev_para* g_RegDev);
uint32_t get_crc_error_cnt(g_RegDev_para* g_RegDev);

/* ----------------------------- get ddr_full_flag , airdata_buf2_empty_flag , airsignal_buf2_empty_flag ---------------------------- */
uint32_t reset_ddr_full_flag(g_RegDev_para* g_RegDev);
uint32_t ddr_full_flag(g_RegDev_para* g_RegDev);
uint32_t airdata_buf2_empty_flag(g_RegDev_para* g_RegDev);
uint32_t airsignal_buf2_empty_flag(g_RegDev_para* g_RegDev);




#endif//GW_CONTROL_H
