#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "gw_tunnel.h"

void show_help(){
    printf("------ help ------- \n");
    printf("example: \n");
    printf("./test -r 192.168.10.1 -l 192.168.10.2 -t iptun1\n");
}

int main(int argc, char *argv[])
{

    char* remote_ip = NULL;
    char* local_ip = NULL;
	char* ip_tunnel = NULL;

	if(argc < 2){
		printf("error input parameters !\n");
		return 0;
	}else{
		if(argv[1] != NULL){
			if(strcmp(argv[1],"-v") == 0){
				printf("version : [%s %s] \n",__DATE__,__TIME__);
				return 0;
			}else if(strcmp(argv[1],"-h") == 0){
                show_help();
				return 0;
            }else if(strcmp(argv[1],"-r") == 0 && strcmp(argv[3],"-l") == 0 && strcmp(argv[5],"-t") == 0){
				if(argv[2] != NULL)
					remote_ip = argv[2];
                if(argv[4] != NULL)
                    local_ip = argv[4];
				if(argv[6] != NULL)
                    ip_tunnel = argv[6];
			}else{
				printf("error parameter\n");
				return 0;
			}
		}
	}

    change_tunnel(remote_ip,local_ip,NULL, ip_tunnel);

}
