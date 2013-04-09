#pragma inline
#include <dos.h>
#include <stdio.h>
#include <time.h>

#define LPT_DATA 0x378


void send_port(int port, unsigned char byte){
	int port_in = port;
	unsigned char byte_in = byte;
	asm {
		cli
		mov dx, port_in
		mov al, byte_in
		out dx, al
		sti
	}
}

#define TIMES 15
#define DLY 50
#define PIN(i) (1 << i)

int main(){
	unsigned int i
		,	pin;

	for(i = 0; i < TIMES; ++i){
		for(pin = 3; pin <= 6; ++pin){
			send_port(LPT_DATA, PIN(pin));
			delay(DLY);
		}
	}
	send_port(LPT_DATA, 0);

	return 0;
}
