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

int main(int argc, char **argv){
	unsigned char word;

	if(argc < 2){
		printf("Co bys chcial? Piwa tu nie ma!\n");
		return -1;
	}
	word = atoi(argv[1]);
	
	send_port(LPT_DATA, word);

	return 0;
}
