#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/socket.h>  
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>  
#include <netinet/in.h>
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
#include "gw_tunnel.h"


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
	strncpy(medium, "iptun", IFNAMSIZ - 1); // ethn 0415
	//strncpy(p->name, *argv, IFNAMSIZ - 1);
	strncpy(p->name, "iptun", IFNAMSIZ - 1); // ethn 0415
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

int do_add(int cmd, char* daddr, char* saddr,char* dual_daddr) // SIOCADDTUNNEL or SIOCCHGTUNNEL
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

int change_tunnel(char* daddr, char* saddr,char* dual_daddr){
	int status = do_add(SIOCCHGTUNNEL,daddr,saddr,dual_daddr);
	return status;
}

void initTunnelSystem(char* script){
	printf("call initTunnelSystem()\n");
	//system("./my_init.sh");
	system(script);
}









