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

#define SIGNAL_TICKS 1

typedef struct {
	unsigned char data
		,	state
		,	control
		,	ir_left
		,	ir_right
		,	cny_left
		,	cny_mid
		,	cny_right
		,	trig
		,	rec
		,	echo
		,	enable;

	int ticks
		,	pre_ticks
		,	post_ticks
		,	signal_ticks
		,	time
		,	range;
	unsigned int cycle_ticks;

} state_t;

state_t state;

void send_port(int port, unsigned char byte){
	int port_in = port;
	unsigned char byte_in = byte;
	asm {
		mov dx, port_in
		mov al, byte_in
		out dx, al
	}
}

unsigned char receive_port(int port){
	unsigned char result;
	int port_in = port;
	asm {
		mov dx, port_in
		in al, dx
		mov result, al
	}
	return result;
}

void light(unsigned char *byte, unsigned char mask){
	*byte |= mask;
}

void dark(unsigned char *byte, unsigned char mask){
	*byte &= ~mask;
}

bool norm_to_bool(bool arg){
	if(arg){
		return true;
	}
	return false;
}

#define HC_SR04 (1 << 6)
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
	state->echo = norm_to_bool(!(state->state & HC_SR04));
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

int fast_tick
	,	slow_tick
	,	tick_counter;
void interrupt (far *oldtimer)();

void deinit_timer();


void init_handler(state_t *state){
	state->trig = false;
	state->enable = true;
	state->ticks = 0;
	state->signal_ticks = 0;
	state->pre_ticks = 0;
	state->post_ticks = 0;
	state->cycle_ticks = 0;

	load(state);
}

void handler(state_t *state){
	load(state);

	if(state->trig){
		state->signal_ticks += 1;
	}
	if(state->signal_ticks >= SIGNAL_TICKS){
		state->trig = false;
		send_port(LPT_DATA, 0);
	}


	if(!state->trig && state->enable){
		state->trig = true;
		state->enable = false;
		send_port(LPT_DATA, 1);
	}

	if(state->echo){
		state->ticks += 1;
		state->rec = true;
	}

	if(state->rec && !state->echo){
		state->rec = false;
		state->time = state->ticks * 10;
		state->range = state->time / 58;
	}

	if(!state->echo){
		if(state->ticks == 0){
			state->pre_ticks += 1;
		}else{
			state->post_ticks += 1;
		}
	}
	
	state->cycle_ticks += 1;
	if(state->cycle_ticks >= 6000){
		state->cycle_ticks = 0;
	}
}

void interrupt new_timer(){
	asm cli
	fast_tick += 1;
	tick_counter += 1;

	handler(&state);

	if( tick_counter >= 5461 ) {
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

void init_timer(){
	slow_tick = 1;
	fast_tick = 1;
	tick_counter = 0;
	oldtimer = getvect(8);

	init_handler(&state);

	asm cli

// 	speed up clock
// 	1193180 / 298295 = 4 (298295kHz, 3,3523861 us, 3 ticks 10,057158us)
//	its 16384 times faster than standard 18.2Hz clock
	asm {
			mov bx, 12
			mov al, 00110100b
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



unsigned int counter = 0
	,	stamp;
state_t state_int;
void interrupt count(){
	asm cli
	counter += 1;
	stamp = state.cycle_ticks;
	load(&state_int);
	asm {
		mov al, 20h
		out 20h, al
	}
	asm sti
}


void interrupt (far *old_func)();
void init(){
	unsigned char s8259;
	printf("Init\n");
	asm cli
	s8259 = receive_port(LPT_CONTROL);
	send_port(LPT_CONTROL, s8259 | (1 << 4));
	old_func = getvect(0x0f);
	setvect(0x0f, count);
	s8259 = receive_port(0x21);
	send_port(0x21, s8259 & ~(1 << 7));
	s8259 = receive_port(0x21);

	asm sti

	printf("8259 mask:\n");
	print_byte(s8259);
	printf("\n\n");
}

void deinit(){
	unsigned char s8259;
	asm cli
	setvect(0x0f, old_func);
	s8259 = receive_port(0x21);
	send_port(0x21, s8259 | (1 << 7));
	asm sti
}


int main(){
	printf("BEGIN TIMESTAMP:  %d\n\n", time(NULL));

	load(&state);

	print_state(&state);

	init();
	init_timer();

	print_state(&state);

	while( slow_tick < 9 ){

	}

	print_state(&state);
//	delay(1000000);

	deinit_timer();
	deinit();

	printf("\n\nSlow ticks: %d\nFast ticks: %d\n\n", slow_tick, fast_tick);
	printf("Pre: %d, Signal: %d, Ticks: %d, post: %d\n", state.pre_ticks, state.signal_ticks, state.ticks, state.post_ticks);
	printf("\nTime: %dus\nRange: %d cm\n", state.time, state.range);

	printf("Ack7: %u, stamp: %u\n", counter, stamp);
	printf("Range from IRQ7: %lf", ((double)(stamp - 49)) * 0.17339928);

	printf("\nState at int7\n");
	print_state(&state_int);

	printf("\n  END TIMESTAMP:  %d\n", time(NULL));

	return 0;
}
