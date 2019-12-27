#include <stdio.h>
#include <time.h>
#include "MT.h"

int main(void){
	int i;
	int max = 100;
	
	init_genrand((unsigned)time(NULL));
	for(int i=0;i<max;i++) {
		printf("%ld\n",genrand_int32());
	}
}
