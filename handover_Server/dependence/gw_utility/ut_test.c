#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include "gw_utility.h"

void test_func()
{
    static int count = 0;
	if(count == 50)
		start_timer(50000);
	if(count == 100)
		start_timer(500000);
	if(count == 200)
		start_timer(0);
    printf("count is %d\n", count++);
}


void mydelay(){
	int i,j;
	for(i=0;i<1000;i++){
		for(j=0;j<1000;j++)
			;
	}
}

int main(int argc, char **argv)
{
	int tick = 0;
    init_sigaction(test_func);
    //start_timer(10000);

    while(1){
		if(tick == 50){
			printf("start_timer\n");
			start_timer(10000);
		}
		printf("log while timer\n");
		tick = tick + 1;
		mydelay();
		//pause();
	}

    return 0;
}
