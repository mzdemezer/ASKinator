#pragma inline
#include <dos.h>
#include <stdio.h>
#include <time.h>

#define BIOSTICK (*(volatile unsigned long far *)(0x0040006CL))
#define LPT_DATA 0x378
#define LPT_STATE 0x389
#define LPT_CONTROL 0x37A
#define bool unsigned char
#define false 0
#define true 1

typedef struct {
	unsigned char data
		,	state
		,	control
		,	ir_left
		,	ir_right
		,	cny_left
		,	cny_mid
		,	cny_right;
	
} state_t;

state_t state;

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

unsigned char receive_port(int port){
	unsigned char result;
	int port_in = port;
	asm {
		cli
		mov dx, port_in
		in al, dx
		mov result, al
		sti
	}
	return result;
}

void update_data(){
	send_port(LPT_DATA, state.data);
}

int fast_tick
	,	slow_tick
	,	tick_counter;
void interrupt (far *oldtimer)();

void deinit_timer();

#define POWER_ENGINE (1 << 7)
#define LEFT_ENGINE (1 << 6)
#define RIGHT_ENGINE (1 << 5)

void light(unsigned char *byte, unsigned char mask){
	*byte |= mask;
}

void dark(unsigned char *byte, unsigned char mask){
	*byte &= ~mask;
}

void enable_power_engine(state_t *state){
	light(&state->data, POWER_ENGINE);
}

void enable_left_engine(state_t *state){
	light(&state->data, LEFT_ENGINE);
}

void enable_right_engine(state_t *state){
	light(&state->data, RIGHT_ENGINE);
}

void disable_power_engine(state_t *state){
	dark(&state->data, POWER_ENGINE);
}

void disable_left_engine(state_t *state){
	dark(&state->data, LEFT_ENGINE);
}

void disable_right_engine(state_t *state){
	dark(&state->data, RIGHT_ENGINE);
}

bool norm_to_bool(bool arg){
	if(arg){
		return true;
	}
	return false;
}

#define CNY_LEFT (1 << 3)
#define CNY_MID (1 << 4)
#define CNY_RIGHT (1 << 5)
#define IR_LEFT (1 << 1)
#define IR_RIGHT (1 << 2)


int main(int argc, char **argv){
	int begin = time(NULL)
		,	pin
		,	offset;

	if(argc < 2){
		printf("Ze ktory pin?\n");
		return -1;
	}
	pin = atoi(argv[1]);
	if(pin < 0
	|| pin > 7){
		printf("Zly numer pinu pajacu\n");
		return -2;
	}

	if(argc < 3){
		offset = 0;
	}else{
		offset = atoi(argv[2]);
	}
	if(offset < 0
	|| offset > 2){
		printf("Wyjezdzasz za LPT jebany idioto!\n");
		offset = 0;
	}

	send_port(LPT_DATA + offset, (1 << pin));
//	send_port(LPT_DATA + 1, 255);
	send_port(LPT_DATA + 1, 0);
//	delay(2000);
// 	while( time(NULL) - begin < 2 ){
// 		; // wait
// 	}

 //	send_port(LPT_DATA, 0);

	return 0;
}
