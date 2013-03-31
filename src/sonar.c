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

#define FREQ_DIVI 12
#define IO_DELAY 2

#define ECHO_START 125


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

	unsigned int cycle_ticks
		,	ticks
		,	time
		,	range
		,	io_tick;



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
	state->echo = norm_to_bool(state->state & HC_SR04);
}

void print_byte(unsigned char byte){
	int i
		,	one = 1;

	for(i = 0; i < 8; ++i, one <<= 1){
		printf(" %u", norm_to_bool(byte & one));
	}
	printf("\n");
}

void print_state(state_t *state){
	printf("\nWHAT\t   0 1 2 3 4 5 6 7\n");
	printf("DATA\t  ");
	print_byte(state->data);

	printf("STAT\t  ");
	print_byte(state->state);

	printf("CTRL\t  ");
	print_byte(state->control);
}

volatile unsigned int fast_tick
	,	slow_tick
	,	tick_counter;
void interrupt (far *oldtimer)();

void deinit_timer();


void init_handler(state_t *state){
	state->trig = false;
	state->enable = true;
	state->ticks = 0;
	state->cycle_ticks = 0;
	state->io_tick = 0;

	load(state);
}

unsigned int debug_sent_signals = 0;
void handler(state_t *state){
	state->io_tick += 1;
	if(state->io_tick >= IO_DELAY){
		state->io_tick = 0;
		load(state);
	}


	if(state->cycle_ticks >= 6000){
		state->cycle_ticks = 0;
//		state->ticks = 0;
	}

	if(state->cycle_ticks == 1){
		send_port(LPT_DATA, 0);
	}


	if(state->cycle_ticks == 0
	&& state->enable){
		state->enable = false;
		send_port(LPT_DATA, 1);
	}

	if(state->echo){
		debug_sent_signals += 1;
	}

	state->cycle_ticks += 1;
}

void interrupt (far *old_ack5)();
void interrupt (far *old_ack7)();

unsigned int int7 = 0;
void ack_int_logic(state_t *state){
	state->ticks = state->cycle_ticks - ECHO_START;
	state->time = (state->ticks / 3) * 10;
	state->range = state->time / 58;
}

void interrupt ack_int_7(){
	asm cli
	if(int7 == 0){
		ack_int_logic(&state);
	}
	int7 += 1;
	asm {
		mov al, 20h
		out 20h, al
	}
	asm sti
}

void init_ack(){
	asm cli

	send_port(LPT_CONTROL, 16);

	old_ack7 = getvect(0x0f);
	setvect(0x0f, ack_int_7);

	asm sti
}

void deinit_ack(){
	asm cli

	send_port(LPT_CONTROL, 0);
	setvect(0x0f, old_ack7);

	asm sti
}

void interrupt new_timer(){
	asm cli
	fast_tick += 1;
	tick_counter += 1;

	handler(&state);

	if( tick_counter >= 5461 ){//16384 ) {
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
			mov bx, FREQ_DIVI
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


int main(){
	//unsigned int last = 0;
	printf("BEGIN TIMESTAMP:  %d\n\n", time(NULL));


	load(&state);

	print_state(&state);


	init_timer();
	init_ack();
	print_state(&state);
	printf("\nSlow ticks: %u\nFast ticks: %u\n", slow_tick, fast_tick);
	delay(500);
	//while( slow_tick < 36 ){
//		if(last != slow_tick){
//			last = slow_tick;
//			printf("%u\n", state.echo);
//		}
	//}
	printf("\nSlow ticks: %u\nFast ticks: %u\n", slow_tick, fast_tick);
	print_state(&state);


	deinit_timer();
	deinit_ack();

	printf("\nSlow ticks: %u\nFast ticks: %u\n", slow_tick, fast_tick);
	printf("Ticks: %d\n", state.ticks);
	printf("\nTime: %dus\nRange: %d cm\n", state.time, state.range);

	printf("Ack7: %u, sent_sigs: %u\n", int7, debug_sent_signals);

	printf("Old ack: %p\n", old_ack7);

	printf("\n  END TIMESTAMP:  %d\n", time(NULL));

	return 0;
}
