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

int main(){

	send_port(LPT_DATA, 0);

	return 0;
}
