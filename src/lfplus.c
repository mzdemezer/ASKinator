#pragma inline
#include <dos.h>
#include <stdio.h>
#include <string.h>

#define LPT_DATA 0x378
#define LPT_STATE 0x379
#define LPT_CONTROL 0x37A
#define bool unsigned char
#define false 0
#define true 1

#define SONAR_ECHO_START 49
#define SONAR_MOD 5966

#define IO_MOD 497

#define CLOCK_MOD 5461


void send_port(int port, unsigned char byte);
unsigned char receive_port(int port);
bool norm_to_bool(unsigned char arg);


void init();
void init_clock();
void init_ack();

void restore();
void restore_clock();
void restore_ack();

/**
 * State structure
 */
typedef struct {
	/**
	 * Odczyt rejestrow LPT
	 */
	unsigned char data
		,	state
		,	control;
	
	/**
	 * Biezacy stan czujnikow
	 */
	bool ir_left
		,	ir_right
		,	cny_left
		,	cny_mid
		,	cny_right
		,	echo;
	
	unsigned int range /** Interpretacja sygnalu sonara na odleglosc */
		,	cycle_ticks /** 0-5999 (mod 6000), licznik tikow zegara (co 10 mikrosekund) */
		,	clock_ticks; /** 0-5461 + 1/3 */
	unsigned char clock_thirds; /** 0-2 (mod 3) dla 1/3 taktu zagara */
	bool clock_edge; /** wskazuje zbocze pierwotnego zegara */
} state_t;
state_t state;

void init_state(state_t *state){
	state->clock_ticks = 0;
	state->clock_thirds = 0;
	state->clock_edge = false;
}

/**
 * Clock
 */
void clock_raise_edge(state_t *state){
	state->clock_edge = true;
}

void clock_fall_edge(state_t *state){
	state->clock_edge = false;
}

void clock_thirds_inc(state_t *state){
	if(state->clock_thirds >= 2){
		state->clock_thirds = 0;
	}else{
		state->clock_thirds += 1;
	}
}

void clock_inc(state_t *state){
	if((state->clock_thirds != 0 && state->clock_ticks == CLOCK_MOD - 1)
	|| (state->clock_thirds == 0 && state->clock_ticks >= CLOCK_MOD) ){
		clock_thirds_inc(state);
		
		state->clock_ticks = 0;
		clock_raise_edge(state);
	}else{
		state_clock_ticks += 1;
	}
}

/**
 * Sonar cycle
 */
void cycle_inc(state_t *state){
	if(state->cycle_ticks == CYCLE_MOD - 1){
		state->cycle_ticks = 0;
	}else{
		state->cycle_ticks += 1;
	}
}

/**
 * Input/output
 */
/**
 * Wysyla   byte   pod   port
 */
void send_port(int port, unsigned char byte){
	int port_in = port;
	unsigned char byte_in = byte;
	asm {
		mov dx, port_in
		mov al, byte_in
		out dx, al
	}
}

/**
 * Odbiera i zwraca bajt spod   portu
 */
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

/**
 * Bit manipulation
 */
/**
 * Zapala w bajcie jedynki z maski
 */
void light(unsigned char *byte, unsigned char mask){
	*byte |= mask;
}

/**
 * Gasi w bajcie jedynki z maski
 */
void dark(unsigned char *byte, unsigned char mask){
	*byte &= ~mask;
}

/**
 * if(arg != 0){
 * 	return 1;
 * }else{
 * 	return 0;
 * }
 */
bool norm_to_bool(unsigned char arg){
	if(arg){
		return true;
	}
	return false;
}

/**
 * Logika
 */
// 	1193180 / 100000 = 11,9318 ~ 12 (99431,667kHz, 10,057158 us)
// 	(liczba_tikow * 10,057158 us) / 58 = dystans [cm]
// 	liczba_tikow * 0,17339928
void ack_int_logic(state_t *state){
	state->range = ((double)(state->cycle_ticks - SONAR_ECHO_START)) * 0.17339928;
}

void clock_int_logic(state_t *state){
	
}

/**
 * Interrupt vectors
 */
/**
 * Potwierdzenie przyjścia przerwania definiowane jako wstawka assemblerowa
 */
#define EOI asm { \
	mov al, 20h \
	out 20h, al \
} \

#define DISABLE_INTERRUPTS asm cli
#define ENABLE_INTERRUPTS asm sti

/**
 * Funkcja zastępujące przerwanie zegarowe IRQ0
 */
void interrupt clock_tick(){
	DISABLE_INTERRUPTS
	
	clock_inc(&state);
	clock_int_logic(&state);
	
	if(state->clock_edge){
		clock_fall_edge(&state);
		
		// old clock
		
	}else{
		EOI
	}
	ENABLE_INTERRUPTS
}

/**
 * Funkcja wykonywana przy przerwaniu IRQ7
 * wywolywanym przez linie ACK z portu LPT
 */
void interrupt sonar_finish_receive(){
	DISABLE_INTERRUPTS
	
	ack_int_logic(&state);
	
	EOI
	ENABLE_INTERRUPTS
}

void init(){
	DISABLE_INTERRUPTS
	
	init_clock();
	init_ack();
	
	ENABLE_INTERRUPTS
}

void init_clock(){
	
}

void restore(){
	DISABLE_INTERRUPTS
	
	restore_clock();
	restore_ack();
	
	ENABLE_INTERRUPTS
}

int main(){
	bool still_work = true;
	char buf[512];
	
	while(still_work){
		scanf(" %s", buf);
		
		if(strncmp(buf, "exit") == 0){
			still_work = false;
		}
		
	}
	
	return 0;
}
