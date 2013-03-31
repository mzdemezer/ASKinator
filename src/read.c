#pragma inline
#include <dos.h>
#include <stdio.h>
#include <time.h>

#define BIOSTICK (*(volatile unsigned long far *)(0x0040006CL))
#define LPT_DATA 0x378
#define LPT_STATE 0x379
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
void load(state_t *state){
	state->data = receive_port(LPT_DATA);
	state->state = receive_port(LPT_STATE);
	state->control = receive_port(LPT_CONTROL);
	
	state->ir_left = norm_to_bool(state->control & IR_LEFT);
	state->ir_right = norm_to_bool(state->control & IR_RIGHT);
	state->cny_left = norm_to_bool(state->state & CNY_LEFT);
	state->cny_mid = norm_to_bool(state->state & CNY_MID);
	state->cny_right = norm_to_bool(state->state & CNY_RIGHT);
}

void print_byte(unsigned char byte){
	int i
		,	one = 1;
	
	for(i = 0; i < 8; ++i, one <<= 1){
		printf(" %u", norm_to_bool(byte & one));
	}
}

void print_state(state_t *state){
	printf("\n\nWHAT\t   0 1 2 3 4 5 6 7\n");
	printf("DATA\t  ");
	print_byte(state->data);

	printf("\nSTAT\t  ");
	print_byte(state->state);

	printf("\nCTRL\t  ");
	print_byte(state->control);
}

int main(int argc, char **argv){
	int begin = time(NULL)
		,	secs;
	unsigned char casted;
	state_t state;

	if(argc == 1){
		secs = 3;
	}else{
		secs = atoi(argv[1]);
	}
	if(secs == 0){
		secs = 3;
	}

	//printf("while in: timestamp %d\n", begin);
	//printf("cond: %d, %d\n", time(NULL) - begin, (time(NULL) - begin) < 3);
	casted = time(NULL) - begin;
	while( casted < secs){
		//printf("while inside\n");

		load(&state);
		print_state(&state);
		delay(300);
		casted = time(NULL) - begin;
	}
	//printf("while out: timestamp %d\n", time(NULL));

	return 0;
}
