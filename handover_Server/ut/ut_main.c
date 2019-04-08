#include <stdio.h>
#include <stdlib.h>
#include "gw_utility.h"
#include "gw_control.h"

// broker callback interface
int process_exception(char* buf, int buf_len, char *from, void* arg)
{
	int ret = 0;
	if(strcmp(from,"mon/all/pub/system_stat") == 0){
		printf("system_stat is %s \n" , buf);
	}
	return ret;
}


int main(int argc, char *argv[]){
	printf("argc = %d \n",argc);
	if(argc!=2)
	{
		printf("Usage : ethname\n");
		return 1;
	}
    char    szMac[18];
    int     nRtn = get_mac(szMac, sizeof(szMac),argv[1]);
    if(nRtn > 0) // nRtn = 12
    {	
        printf("argv[1] = %s , nRtn = %d , MAC ADDR : %s\n", argv[1],nRtn,szMac);
    }else{
		return 0;
	}
	//char* tmp = "000129d46f68";
	//char* tmp = "100f000046f0";
	//printf("str = %s\n",tmp);

	int ret = initBroker(argv[0],process_exception);
	printf("initBroker : ret = %d \n", ret);

	char* dst = "000A35000123";
	char* source = "000A35000122"; // source

	char source_aaa[6]; // source
	change_mac_buf(source,source_aaa);
	hexdump(source_aaa,6);
	char* high16str = getHigh16Str(source_aaa);
	printf("high16str = %s\n",high16str);
	char* low32str = getLow32Str(source_aaa);
	printf("low32str = %s\n",low32str);

	ret = set_dst_mac(low32str, high16str);
	printf("set_dst_mac : ret = %d \n", ret);
	free(high16str);
	free(low32str);

	char dst_aaa[6]; // dst
	change_mac_buf(dst,dst_aaa);
	hexdump(dst_aaa,6);
	high16str = getHigh16Str(dst_aaa);
	printf("high16str = %s\n",high16str);
	low32str = getLow32Str(dst_aaa);
	printf("low32str = %s\n",low32str);


	ret = set_src_mac(low32str, high16str);
	printf("set_src_mac : ret = %d \n", ret);

	free(high16str);
	free(low32str);
}
