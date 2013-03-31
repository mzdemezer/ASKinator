#pragma inline
#include <dos.h>
#include <stdio.h>

unsigned int counter = 0;
void interrupt count(){
	counter += 1;
}


void interrupt (far *old_func)();
void init(){
	old_func = getvect(0x0f);
	setvect(0x0f, count);
}

void deinit(){
	setvect(0x0f, old_func);
}

int main(){

	init();

	delay(1000);

	deinit();

	printf("%u\n", counter);



	return 0;
}