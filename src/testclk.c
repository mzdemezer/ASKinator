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

//#define divi 1193180
//#define FREQ 1820
//#define max_div 65536
#define FREQ 656
#define FASTER 100
//#define FREQ8253 (divi / FREQ)

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

void logic(state_t *state){
	bool xor_sides = state->cny_left ^ state->cny_right;
	
	if(xor_sides){
		if(state->cny_right){
			disable_left_engine(state);
			enable_right_engine(state);
		}else{
			enable_left_engine(state);
			disable_right_engine(state);
		}
	}else{
// 		if(!state->cny_mid){
			enable_left_engine(state);
			enable_right_engine(state);
// 		}
	}
	
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
	state->state = receive_port(LPT_STATE);
	state->control = receive_port(LPT_CONTROL);
	
	state->ir_left = norm_to_bool(state->control & IR_LEFT);
	state->ir_right = norm_to_bool(state->control & IR_RIGHT);
	state->cny_left = norm_to_bool(state->state & CNY_LEFT);
	state->cny_mid = norm_to_bool(state->state & CNY_MID);
	state->cny_right = norm_to_bool(state->state & CNY_RIGHT);
}

void handler(){
	load(&state);
	
	logic(&state);
	
	update_data();
}

void interrupt new_timer(){
	print_state(&state);

	asm cli
	fast_tick += 1;
	tick_counter += 1;

	handler();

	if( tick_counter >= FASTER ) {
		slow_tick += 1;
		tick_counter = 0;
		oldtimer();
	}else{
		// reset PIC
		asm {
			mov al, 20h
			out 20h, al
		}
	}
	asm sti

}

void init_logic(state_t *state){
	enable_power_engine(state);
}

void init_handler(){
	init_logic(&state);
	
	update_data();
}

void init_timer(){
	slow_tick = 1;
	fast_tick = 1;
	tick_counter = 0;
	oldtimer = getvect(8);
	
	init_handler();
	
	asm cli

	// speed up clock
	asm {
			mov bx, FREQ /*19886 /* 60Hz */
			mov al, 00110110b
			out 43h, al
			mov al, bl
			out 40h, al
			mov al, bh
			out 40h, al
	}
	
	setvect(8, new_timer);
	
	asm sti
}

void deinit_timer(){
	asm cli

	// slow down clock 1193180 / 65536 = 18.2, but we use zero

	asm {
		xor bx, bx
		mov al, 00110110b
		out 43h, al
		mov al, bl
		out 40h, al
		mov al, bh
		out 40h, al
	}

	setvect(8, oldtimer); // restore oldtimer

	asm sti

}

int main(){
	int begin = time(NULL);
	char c;
	printf("Begin: %d\n", begin);
	printf("init_timer in\n");
	init_timer();
	printf("init_timer out\n");
// 	printf("pre loop>\tfast_tick: %d\n", fast_tick);
	
	
	scanf("%c", &c);
	printf("Your termination character is %c\n", c);
	//while( fast_tick <= 18.2 * FASTER * 10 ){
		
	//}
// 	printf("post loop>\tfast_tick: %d\n", fast_tick);
	
	printf("deinit_timer in\n");
	deinit_timer();
	printf("deinit_timer out\n");
	
	// stop
	send_port(LPT_DATA, 0);

// 	printf("Timestamp: %d", time(NULL) - begin);
// 	printf("main out>\tfast_tick: %d\n", fast_tick);
	return 0;
}
