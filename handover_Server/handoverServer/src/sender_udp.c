//Client.c
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/socket.h>  
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>  
#include <netinet/in.h> 
#include "Pattern.h"
#include <time.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/in_route.h>
#include <asm/types.h>
#include <netinet/in.h>
#include <unistd.h>

#include "If_tunnel.h"
//#include <linux/if_tunnel.h>
#define PORT 44444  

#define MAX_BYTE 5000

static int get_addr_ipv4(__u8 *ap, const char *cp)
{
	int i;

	for (i = 0; i < 4; i++) {
		unsigned long n;
		char *endp;

		n = strtoul(cp, &endp, 0); // stdlib.h
		if (n > 255)
			return -1;	/* bogus network value */

		if (endp == cp) /* no digits */
			return -1;

		ap[i] = n;

		if (*endp == '\0')
			break;

		if (i == 3 || *endp != '.')
			return -1; 	/* extra characters */
		cp = endp + 1;
	}

	return 1;
}

int get_addr_1(inet_prefix *addr, const char *name, int family)
{
	addr->family = AF_INET;
	if (family != AF_UNSPEC && family != AF_INET)
		return -1;

	if (get_addr_ipv4((__u8 *)addr->data, name) <= 0)
		return -1;

	addr->bytelen = 4;
	addr->bitlen = -1;
	return 0;
}

__u32 get_addr32(const char *name)
{
	inet_prefix addr;
	if (get_addr_1(&addr, name, AF_INET)) {
		fprintf(stderr, "Error: an IP address is expected rather than \"%s\"\n", name);
		exit(1);
	}
	return addr.data[0];
}

int tnl_add_ioctl(int cmd, const char *basedev, const char *name, void *p)
{
	struct ifreq ifr;
	int fd;
	int err;

	if (cmd == SIOCCHGTUNNEL && name[0])
		strncpy(ifr.ifr_name, name, IFNAMSIZ);
	else
		strncpy(ifr.ifr_name, basedev, IFNAMSIZ);
	ifr.ifr_ifru.ifru_data = p;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		fprintf(stderr, "create socket failed: %s\n", strerror(errno));
		return -1;
	}

	err = ioctl(fd, cmd, &ifr);
	if (err)
		fprintf(stderr, "add tunnel \"%s\" failed: %s\n", ifr.ifr_name,
			strerror(errno));
	close(fd);
	return err;
}

static int parse_args(char* daddr, char* saddr, char* dual_daddr, int cmd, struct ip_tunnel_parm *p) // dual_daddr == NULL -- single;  dual_daddr!=NULL -- dual 
{
	char medium[IFNAMSIZ] = {};
	memset(p, 0, sizeof(*p));
	p->iph.version = 4;
	p->iph.ihl = 5;
#ifndef IP_DF
#define IP_DF		0x4000		/* Flag: "Don't Fragment"	*/
#endif
	p->iph.frag_off = htons(IP_DF);

	p->iph.protocol = IPPROTO_IPIP;

	p->iph.daddr = get_addr32(daddr);//get_addr32(*argv);

	p->iph.saddr = get_addr32(saddr);//get_addr32(*argv);


	//__be16	id;
	//__be16    tot_len;
	//__u8	tos; __u8	ttl;

	if(dual_daddr != NULL){
		__be32 dual_destination = get_addr32(dual_daddr);
		p->iph.tot_len = (dual_destination & 0xffff0000) >> 16; // high 16
		p->iph.id = dual_destination & 0xffff;                  // low  16
		printf("dual_destination = %u , p->iph.tot_len =%u, p->iph.id = %u \n",dual_destination,p->iph.tot_len,p->iph.id);
	}
	printf("get_addr32(daddr) = %u \n", p->iph.daddr);

	//strncpy(medium, *argv, IFNAMSIZ - 1); // (strcmp(*argv, "dev") == 0)
	strncpy(medium, "ethn", IFNAMSIZ - 1);
	//strncpy(p->name, *argv, IFNAMSIZ - 1);
	strncpy(p->name, "ethn", IFNAMSIZ - 1);
/*
	if (medium[0]) {
		p->link = 3;//ll_name_to_index(medium); // ????
		if (p->link == 0) {
			fprintf(stderr, "Cannot find device \"%s\"\n", medium);
			return -1;
		}
	}
*/
	if (p->i_key == 0 && IN_MULTICAST(ntohl(p->iph.daddr))) {
		p->i_key = p->iph.daddr;
		p->i_flags |= GRE_KEY;
	}
	if (p->o_key == 0 && IN_MULTICAST(ntohl(p->iph.daddr))) {
		p->o_key = p->iph.daddr;
		p->o_flags |= GRE_KEY;
	}
	if (IN_MULTICAST(ntohl(p->iph.daddr)) && !p->iph.saddr) {
		fprintf(stderr, "A broadcast tunnel requires a source address\n");
		return -1;
	}

	return 0;
}

static int do_add(int cmd, char* daddr, char* saddr,char* dual_daddr) // SIOCADDTUNNEL or SIOCCHGTUNNEL
{
	struct ip_tunnel_parm p;
	const char *basedev;

	if (parse_args(daddr, saddr, dual_daddr, cmd, &p) < 0)
		return -1;

	if (p.iph.ttl && p.iph.frag_off == 0) {
		fprintf(stderr, "ttl != 0 and nopmtudisc are incompatible\n");
		return -1;
	}

	basedev = "tunl0";
	if (!basedev) {
		fprintf(stderr,
			"cannot determine tunnel mode (ipip, gre, vti or sit)\n");
		return -1;
	}

	return tnl_add_ioctl(cmd, basedev, p.name, &p);
}

	char* d_ip_setup = "1.1.1.1";
	char* s_ip_setup = "2.2.2.2";
	char* remote = "10.0.1.4";
	char* local = "10.0.1.3";
	char* remote_1 = "10.0.1.2";
	char* test_remote = "255.255.255.255";



const char* ip = "10.1.1.101";
 void delay(){
	 int i = 0;
	for(i=0;i<100;i++){
		for(int j= 0; j< 50; j++){}
	}
}

void makeHeader(char * SendBuf, int start, int length, int number)
 {	 
	char ch_number[4];
	*((int*)ch_number) = number;
    memcpy(SendBuf ,ch_number,4);
     
	int  data_temp1 = start;
	char data_start[4];
	*((int *)data_start) = data_temp1;
	memcpy(SendBuf + 4 ,data_start,4);
	
	int  data_temp2 = length;
	char data_length[4];
	*((int *)data_length) = data_temp2;
	memcpy(SendBuf + 8,data_length,4);
}
 
 
int main(int argc,char **argv)  
{  
	char* remote_arry[2];
	remote_arry[0] = remote;
	remote_arry[1] = remote_1;
	do_add(SIOCCHGTUNNEL,remote_1,local,remote_1); // static int do_add(int cmd, char* daddr, char* saddr,char* dual_daddr)

    int sockfd;  
    int err,n;  
	int count = 0;
	int error_count = 0;
    struct sockaddr_in addr_ser;  
      
    sockfd = socket(AF_INET,SOCK_DGRAM,0); 
    if(sockfd == -1)  
    {  
        printf("socket error\n");  
        return -1;  
    }  
      
    bzero(&addr_ser,sizeof(addr_ser));       
       
    addr_ser.sin_family = AF_INET;    
    addr_ser.sin_port = htons(PORT); 
	inet_pton(AF_INET, ip, &addr_ser.sin_addr.s_addr);                 
       
	int sendbuflen=32 * 1024 * 1024;
	setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,(const char*)&sendbuflen,sizeof(int));
	
    printf("start send...\n");  

      
    //数据传输  
	unsigned int sendTime = 3000001;
	unsigned int seed = time(0);
	srand(seed);					//按时间随机
	
	int   BufLen = MAX_BYTE; // 
	char  SendBuf[MAX_BYTE];
    memset(SendBuf, 0 , sizeof(SendBuf));
	int start;
	int length;
    char *srcData_start;
	int index_tunnel = 0;
    while(--sendTime)  
    {
		start  = 3;//rand() % 1520;
		length = 1300;//rand() % 1400 + 12 + 8 + 20 = 1440;
		
		count = count + 1;
		
		makeHeader(SendBuf,start,length,count);

		srcData_start = srcData + start;
		
		memcpy(SendBuf + 12, srcData_start, length);

		//if(count % 2000 == 0){
			//do_add(SIOCCHGTUNNEL,remote_arry[index_tunnel],local);
			//index_tunnel = (index_tunnel + 1) % 2;
			//printf("change ----- tunnel\n");
			//return;
		//}
		
        int n = sendto(sockfd, SendBuf,length + 12,0,(struct sockaddr *)&addr_ser,sizeof(addr_ser));
		if(n != (length + 12)){
			error_count = error_count + 1;		
		}
		//delay();
    }
	printf("count = %d\n",count);
	printf("error_count = %d\n",error_count);
	unsigned int sendTime_end = 201;
	while(--sendTime_end){
		makeHeader(SendBuf,start,length,0xffffffff);
		//if(sendTime_end%1000 == 0){
			//do_add(SIOCCHGTUNNEL,remote_arry[index_tunnel],local);
			//index_tunnel = (index_tunnel + 1) % 2;
		//}
		sendto(sockfd, SendBuf,length + 12,0,(struct sockaddr *)&addr_ser,sizeof(addr_ser));
	}
	close(sockfd);
    return 0;  
} 
