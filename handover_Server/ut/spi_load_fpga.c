/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/time.h>
#include <dirent.h>


/* my file */

#define PROG_B_GPIO    1023 // out
#define INIT_B_GPIO    990  // inout
#define FPGA_DONE_GPIO 991  // inout

#define GPIO_FPGA_PROG_B_VAL          "/sys/class/gpio/gpio1023/value"
#define GPIO_FPGA_INIT_B_VAL          "/sys/class/gpio/gpio990/value"
#define GPIO_FPGA_DONE_VAL            "/sys/class/gpio/gpio991/value" 

#define SYSFS_GPIO_RST_VAL_H        "1"
#define SYSFS_GPIO_RST_VAL_L        "0"

typedef struct g_gpio_fd_set{
	int fpga_prog_b;
	int fpga_init_b;
	int fpga_done;
}g_gpio_fd_set;


//	===========================
	/*	
	int64_t start = now();
	.....
	int64_t end = now();
	double sec = (end-start)/1000000.0;
	printf("%f sec %f ms \n", sec, 1000*sec);
	*/
//  ===========================
int64_t now()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

int gpio_read(int pin);

void print_status(g_gpio_fd_set* g_gpio_fd){
	printf("print_status ----------------------- \n");
	int state = -1;
	state = gpio_read(INIT_B_GPIO);
	printf("INIT_B_GPIO : %d \n", state);
	state = gpio_read(PROG_B_GPIO);
	printf("PROG_B_GPIO : %d \n", state);
	state = gpio_read(FPGA_DONE_GPIO);
	printf("FPGA_DONE_GPIO : %d \n", state);
}


int set_fpga_prog_b(g_gpio_fd_set* g_gpio_fd, char* value){
	int rc = 0;
	rc = write(g_gpio_fd->fpga_prog_b, value, sizeof(value));
	if(rc < 0){
		printf("error in set_fpga_prog_b , rc = %d \n", rc);
	}
	return rc;
}

int set_fpga_init_b(g_gpio_fd_set* g_gpio_fd, char* value){
	int rc = 0;
	rc = write(g_gpio_fd->fpga_init_b, value, sizeof(value));
	if(rc < 0){
		printf("error in set_fpga_init_b , rc = %d \n", rc);
	}
	return rc;
}

int set_fpga_done(g_gpio_fd_set* g_gpio_fd, char* value){
	int rc = 0;
	rc = write(g_gpio_fd->fpga_done, value, sizeof(value));
	if(rc < 0){
		printf("error in set_fpga_done , rc = %d \n", rc);
	}
	return rc;
}

int gpio_read(int pin)
{  
    char path[128];  

    char value_str[3];  

    int fd;  

	/* /sys/class/gpio/gpio973/value */
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
	
    fd = open(path, O_RDONLY);  

    if (fd < 0){
        printf("Failed to open gpio value for reading!\n");
        return -1;
    }  

    if (read(fd, value_str, 3) < 0){
        printf("Failed to read value!\n");
        return -1;
    }
    close(fd);
    return (atoi(value_str));
}

int filelength(FILE *fp)
{
	int num;
	fseek(fp,0,SEEK_END);
	num=ftell(fp);
	fseek(fp,0,SEEK_SET);
	return num;
}

char* readfile(const char *path, int *len)
{
	FILE *fp;
	int length;
	char *ch;
	if((fp=fopen(path,"r"))==NULL)
	{
		return NULL;
	}
	length=filelength(fp);
	ch=(char *)malloc(length+1);
	fread(ch,length,1,fp);
	*(ch+length)='\0';
	fclose(fp);
	*len = length;
	return ch;
}

int is_dir_exist(const char* dir_path){
    if(dir_path == NULL){
        return -2;
    }
    if(opendir(dir_path) == NULL){
        return -1;
    }
    return 0;
}

void init_gpio_script(){
	char * Path_990="/sys/class/gpio/gpio990/";   // init_b
	char * Path_991="/sys/class/gpio/gpio991/";   // done
	char * Path_1023="/sys/class/gpio/gpio1023/"; // prog_b
	/* fpga_init_b */
	if(is_dir_exist(Path_990) == -1){
		printf("\n  create gpio990 \n");
		system("echo 990 > /sys/class/gpio/export");
		system("echo in > /sys/class/gpio/gpio990/direction");
		//system("echo 1 > /sys/class/gpio/gpio990/value");
	}

	/* fpga_done */
	if(is_dir_exist(Path_991) == -1){
		printf("\n  create gpio991 \n");
		system("echo 991 > /sys/class/gpio/export");
		system("echo in > /sys/class/gpio/gpio991/direction");
		//system("echo 0 > /sys/class/gpio/gpio991/value");
	}

	/* fpga_prog_b */
	if(is_dir_exist(Path_1023) == -1){
		printf("\n  create gpio1023 \n");
		system("echo 1023 > /sys/class/gpio/export");
		system("echo out > /sys/class/gpio/gpio1023/direction");
		system("echo 1 > /sys/class/gpio/gpio1023/value");
	}
}

int init_gpio_fd(g_gpio_fd_set** g_gpio_fd){
	*g_gpio_fd = (g_gpio_fd_set*)malloc(sizeof(struct g_gpio_fd_set));
	(*g_gpio_fd)->fpga_init_b=open(GPIO_FPGA_INIT_B_VAL,O_RDWR);
	if((*g_gpio_fd)->fpga_init_b == -1){
		printf(" fpga_init_b fd open err !!\n");
		return -1;
	}
	(*g_gpio_fd)->fpga_prog_b=open(GPIO_FPGA_PROG_B_VAL,O_RDWR);
	if((*g_gpio_fd)->fpga_prog_b == -1){
		printf(" fpga_prog_b fd open err !!\n");
		return -1;
	}
	(*g_gpio_fd)->fpga_done=open(GPIO_FPGA_DONE_VAL,O_RDWR);
	if((*g_gpio_fd)->fpga_done == -1){
		printf(" fpga_done fd open err !!\n");
		return -1;
	}
	return 0;
}

/* --------------------------------------------------------------------- */

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev1.0";
static uint32_t mode;
static uint8_t bits = 8;
static char *input_file;
static char *output_file;
static uint32_t speed = 24000000;
static uint16_t delay;
static int verbose;

char *input_tx;

static void hex_dump(const void *src, size_t length, size_t line_size,
		     char *prefix)
{
	int i = 0;
	const unsigned char *address = src;
	const unsigned char *line = address;
	unsigned char c;

	printf("%s | ", prefix);
	while (length-- > 0) {
		printf("%02X ", *address++);
		if (!(++i % line_size) || (length == 0 && i % line_size)) {
			if (length == 0) {
				while (i++ % line_size)
					printf("__ ");
			}
			printf(" | ");  /* right close */
			while (line < address) {
				c = *line++;
				printf("%c", (c < 33 || c == 255) ? 0x2E : c);
			}
			printf("\n");
			if (length > 0)
				printf("%s | ", prefix);
		}
	}
}

static void transfer(int fd, uint8_t const *tx, uint8_t const *rx, size_t len)
{
	int ret;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = len,
		.delay_usecs = 5,//delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	if (mode & SPI_TX_QUAD){
		printf("mode & SPI_TX_QUAD \n");
		tr.tx_nbits = 4;
	}
	else if (mode & SPI_TX_DUAL){
		printf("mode & SPI_TX_DUAL \n");
		tr.tx_nbits = 2;
	}
	if (mode & SPI_RX_QUAD){
		printf("mode & SPI_RX_QUAD \n");
		tr.rx_nbits = 4;
	}
	else if (mode & SPI_RX_DUAL){
		printf("mode & SPI_RX_DUAL \n");
		tr.rx_nbits = 2;
	}
	if (!(mode & SPI_LOOP)) {
		if (mode & (SPI_TX_QUAD | SPI_TX_DUAL)){
			printf("mode & (SPI_TX_QUAD | SPI_TX_DUAL \n");
			tr.rx_buf = 0;
		}
		else if (mode & (SPI_RX_QUAD | SPI_RX_DUAL)){
			printf("mode & (SPI_RX_QUAD | SPI_RX_DUAL \n");
			tr.tx_buf = 0;
		}
	}

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

}

int main(int argc, char *argv[])
{
	char* fpga_file = NULL;
	int fpga_file_len = 0;
	if(argc < 2){
		printf("error input parameters !\n");
		return 0;
	}else{
		if(argv[1] != NULL){
			if(strcmp(argv[1],"-v") == 0){
				printf("version : [%s %s] \n",__DATE__,__TIME__);
				return 0;
			}
			else if(strcmp(argv[1],"-p") == 0){
				if(argv[2] != NULL)
					fpga_file = readfile(argv[2],&fpga_file_len);
			}else{
				printf("error parameter\n");
				return 0;
			}
		}
	}

	init_gpio_script();

	/* pre process file header */
	int start_bitfile_pos = -1;
	char* start_bitfile = NULL;
	int start_bitfile_len = 0;
	int cnt_ff = 0;
	int idx_ff = 0;
	for(int i=0;i<fpga_file_len;i++){
		if(fpga_file[i] == 0xff){
			idx_ff = i + 1;
			cnt_ff++;
			while(idx_ff<fpga_file_len && fpga_file[idx_ff] == 0xff){
				idx_ff++;
				cnt_ff++;
				if(cnt_ff == 64){
					start_bitfile_pos = idx_ff;
					start_bitfile_len = fpga_file_len - start_bitfile_pos + 64;
					start_bitfile = fpga_file + start_bitfile_pos - 64;
					break;
				}
			}
			if(start_bitfile_pos != -1){
				break;
			}
		}
	}
	/* end pre process */

	//hex_dump(start_bitfile, 1024, 32, "START_BIT_FILE");

	g_gpio_fd_set* g_gpio_fd = NULL;

	int state = init_gpio_fd(&g_gpio_fd);
	if(state != 0){
		printf("init_gpio_fd failed !\n");
		return 0;
	}

	// reset fpga_prog_b
	set_fpga_prog_b(g_gpio_fd, SYSFS_GPIO_RST_VAL_L);
	usleep(1000);
	set_fpga_prog_b(g_gpio_fd, SYSFS_GPIO_RST_VAL_H);

	/* spi init */

	int ret = 0;
	int fd;

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	mode = mode | SPI_MODE_3;
	printf("spi mode: 0x%x\n", mode);
	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: 0x%x\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);


	// start fpga configuration
	set_fpga_prog_b(g_gpio_fd, SYSFS_GPIO_RST_VAL_L);
	// wait INIT_B_GPIO go down ?
	for(;;){
		int state = -1;
		state = gpio_read(INIT_B_GPIO);
		if(state == 0){
			usleep(1000);
			printf("wati init_b goes down \n");
			break;
		}
	}

	set_fpga_prog_b(g_gpio_fd, SYSFS_GPIO_RST_VAL_H);
	
	for(;;){
		int state = -1;
		state = gpio_read(INIT_B_GPIO);
		if(state == 1){
			usleep(1000);
			printf("wait init_b goes up \n");
			break;
		}
	}

	printf("\n --- transfer fpga configuration --- \n");

	int64_t start = now();


#define BUFF_LEN 256

	uint8_t default_rx[BUFF_LEN] = {0, };
	uint8_t padd_tx[BUFF_LEN] = {0, };

	while(1){
		int transfered_len = BUFF_LEN;
		int sum_transfer_len = 0;
		uint8_t* fpga_file_tmp = (uint8_t*)start_bitfile;

		for(;;){
			if(start_bitfile_len - sum_transfer_len > BUFF_LEN){
				transfered_len = BUFF_LEN;
			}else{
				transfered_len = start_bitfile_len - sum_transfer_len;
				printf("\n transfered_len = %d , BUFF_LEN = %d \n", transfered_len,BUFF_LEN);
			}
			transfer(fd, fpga_file_tmp, default_rx, transfered_len);
			sum_transfer_len = sum_transfer_len + transfered_len;
			fpga_file_tmp = fpga_file_tmp + transfered_len;
			if(sum_transfer_len == start_bitfile_len){
				break;
			}
		}

		int cnt_done = 0;
		int cont_conf = 0;
		for(;;){
			cnt_done = cnt_done + 1;
			if(gpio_read(FPGA_DONE_GPIO) == 1){
				printf(" fpga_done == 1 \n");
				cont_conf = 0;
				break;
			}
			printf("check fpga_done \n");
			sleep(1);
			if(cnt_done == 120){
				cont_conf = 1;
				break;
			}
		}

		if(cont_conf == 0){
			printf("\n tranfer more clock \n");
			for(int i=0;i<1024;i++)
				transfer(fd, padd_tx, default_rx, 32);
			break;
		}

		// ..................
		printf("\n once more fpga configuration !!!\n");
	}

	close(fd);

	double sec = (now()-start)/1000000.0;
	printf("%f sec %f ms \n", sec, 1000*sec);

	printf("\n ---- completed fpga configuration ---- \n");

	return ret;
}
