#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include <math.h>

#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

#include "zlog.h"
#include "adlist.h"
#include "sds.h"
#include "dict.h"


zlog_category_t * serverLog(const char* path){
	int rc;
	zlog_category_t *zlog_handler = NULL;
	rc = zlog_init(path);

	if (rc) {
		return NULL;
	}

	zlog_handler = zlog_get_category("baseStation");

	if (!zlog_handler) {
		zlog_fini();
		return NULL;
	}

	return zlog_handler;
}

void closeServerLog(){
	zlog_fini();
}


/*  ----------------------------------------------------------------------------------  */
 
// int main(int argc, char *argv[]){

// 	zlog_category_t *zlog_handler = serverLog("../conf/zlog_default.conf");

// 	fflush(stdout);
//     setvbuf(stdout, NULL, _IONBF, 0);
// 	zlog_info(zlog_handler,"Start Process \n");
	
// 	int i,j;
// 	int *value_array = (int*)malloc(10*sizeof(int));
// 	for(i=0;i<10;i++){
// 		value_array[i] = i;
// 	}
// 	int new_value = 20;

// 	char* str = "test simple dynamic string";
// 	sds tmp_str = sdsnew(str);
// 	printf("new sds string : %s \n", tmp_str);


// 	for (j = 0; j < 10; j++) {
//         sds long_str = sdsfromlonglong(j);
// 		printf("long_str : %s , %d \n", long_str,j);
//     }
// }










