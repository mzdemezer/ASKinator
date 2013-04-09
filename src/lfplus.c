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

#define PIN(i) (1 << i)

/**
 * Naglowek
 */

/**
 * Struktura przechowujaca stan
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
		,	cycle_ticks /** 0-5965 (mod 5966), licznik tikow zegara (co ok. 10 mikrosekund daje rowno 60ms) */
		,	io_ticks /** 0-496 (mod 497), licznik dla wczytywania stanu czujnikow (co ok. 5 ms - to daje 200 razy na sekunde) */
		,	clock_ticks; /** 0-5461 + 1/3  -  tyle tikow szybszego zegara potrzeba na jeden pierwotnego */
	unsigned char clock_thirds; /** 0-2 (mod 3) dla 1/3 taktu zagara */
	bool clock_edge; /** wskazuje zbocze pierwotnego zegara */
} state_t;
state_t state;

/**
 * Struktura przechowujaca pierwotne
 * funkcje stany scalakow
 */
typedef struct {
	unsigned char old_lpt_ctrl
		,	old_8259_state;
	
	void interrupt (far *old_clock)();
	void interrupt (far *old_ack_int)();
} primary_state_t;
primary_state_t primary_state;
void send_port(int port, unsigned char byte);
unsigned char receive_port(int port);
bool norm_to_bool(unsigned char arg);


void init(state_t *state, primary_state_t *primary_state);
void init_state(state_t *state);
void init_ack(primary_state_t *primary_state);
void init_clock(primary_state_t *primary_state);

void restore(primary_state_t *primary_state);
void restore_ack(primary_state_t *primary_state);
void restore_clock(primary_state_t *primary_state);

/**
 * Zegar
 */
void clock_raise_edge(state_t *state);
void clock_fall_edge(state_t *state);
void clock_thirds_inc(state_t *state);
void clock_inc(state_t *state);


void init();
void init_state(state_t *state);
void init_clock();
void init_ack();

void restore();
void restore_clock();
void restore_ack();





/**
 * Bit manipulation
 */
/**
 * Zapala w bajcie jedynki z maski
 */
#define LIGHT(byte, mask) ((byte) | (mask))
void light(unsigned char *byte, unsigned char mask){
	*byte |= mask;
}

/**
 * Gasi w bajcie jedynki z maski
 */
#define DARK(byte, mask) ((byte) & (~mask))
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
		state->clock_ticks += 1;
	}
}

/**
 * Sonar cycle
 */
void mod_counter(unsigned int *counter, unsigned int modulo){
	if(*counter == modulo - 1){
		*counter = 0;
	}else{
		*counter += 1;
	}
}

void cycle_inc(state_t *state){
	mod_counter(&state->cycle_ticks, SONAR_MOD);
}

void io_inc(state_t *state){
	mod_counter(&state->io_ticks, IO_MOD);
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


#define SONAR_ECHO PIN(6)
#define CNY_LEFT PIN(3)
#define CNY_MID PIN(4)
#define CNY_RIGHT PIN(5)
#define IR_LEFT PIN(7)
#define IR_RIGHT PIN(2)
void load(state_t *state){
	state->data = receive_port(LPT_DATA);
	state->state = receive_port(LPT_STATE);
	state->control = receive_port(LPT_CONTROL);

	state->ir_left = norm_to_bool(!(state->state & IR_LEFT));
	state->ir_right = norm_to_bool(!(state->control & IR_RIGHT));
	state->cny_left = norm_to_bool(state->state & CNY_LEFT);
	state->cny_mid = norm_to_bool(state->state & CNY_MID);
	state->cny_right = norm_to_bool(state->state & CNY_RIGHT);
	state->echo = norm_to_bool(!(state->state & SONAR_ECHO));
}

void update_data(state_t *state){
	send_port(LPT_DATA, state->data);
}


#define SONAR_TRIGGER PIN(0)
void sonar_trigger_high(state_t *state){
	light(&state->data, SONAR_TRIGGER);
}

void sonar_trigger_low(state_t *state){
	dark(&state->data, SONAR_TRIGGER);
}


#define ENGINE_RIGHT_FORWARD PIN(3)
#define ENGINE_RIGHT_BACKWARD PIN(4)
#define ENGINE_LEFT_FORWARD PIN(5)
#define ENGINE_LEFT_BACKWARD PIN(6)
void engines_forward(state_t *state){
	dark(&state->data
		, ENGINE_LEFT_BACKWARD
		| ENGINE_RIGHT_BACKWARD);
	light(&state->data
		, ENGINE_LEFT_FORWARD
		| ENGINE_RIGHT_FORWARD);
}

void engines_backward(state_t *state){
	dark(&state->data
		, ENGINE_LEFT_FORWARD
		| ENGINE_RIGHT_FORWARD);
	light(&state->data
		, ENGINE_LEFT_BACKWARD
		| ENGINE_RIGHT_BACKWARD);
}

void engines_stop(state_t *state){
	dark(&state->data
		, ENGINE_LEFT_BACKWARD
		| ENGINE_LEFT_FORWARD
		| ENGINE_RIGHT_BACKWARD
		| ENGINE_RIGHT_FORWARD);
}

void engines_left_soft(state_t *state){
	dark(&state->data
		, ENGINE_LEFT_FORWARD
		| ENGINE_LEFT_BACKWARD
		| ENGINE_RIGHT_BACKWARD);
	light(&state->data
		, ENGINE_RIGHT_FORWARD);
}

void engines_left_sharp(state_t *state){
	dark(&state->data
		, ENGINE_LEFT_FORWARD
		| ENGINE_RIGHT_BACKWARD);
	light(&state->data
		, ENGINE_RIGHT_FORWARD
		| ENGINE_LEFT_BACKWARD);
}

void engines_right_soft(state_t *state){
	dark(&state->data
		, ENGINE_RIGHT_FORWARD
		| ENGINE_LEFT_BACKWARD
		| ENGINE_RIGHT_BACKWARD);
	light(&state->data
		, ENGINE_LEFT_FORWARD);
}

void engines_right_sharp(state_t *state){
	dark(&state->data
		, ENGINE_RIGHT_FORWARD
		| ENGINE_LEFT_BACKWARD);
	light(&state->data
		, ENGINE_LEFT_FORWARD
		| ENGINE_RIGHT_BACKWARD);
}

/**
 * Logika
 */
// 	1193180Hz / 100000 = 11,9318 ~ 12 (99431,667kHz, 10,057158 us)
// 	(liczba_tikow * 10,057158 us) / 58 = dystans [cm]
// 	liczba_tikow * 0,17339928
void ack_int_logic(state_t *state){
	state->range = ((double)(state->cycle_ticks - SONAR_ECHO_START)) * 0.17339928;
}

void move_logic(state_t *state){
	if(state->range < 20.0){
		engines_stop(state);
	}else{
		if(state->cny_left ^ state->cny_right){
			if(state->cny_mid){
				if(state->cny_left){
					engines_right_sharp(state);
				}else{
					engines_left_sharp(state);
				}
			}else{
				if(state->cny_left){
					engines_right_soft(state);
				}else{
					engines_left_soft(state);
				}
			}
		}else if(!state->cny_mid
		&& !state->cny_left){
			engines_forward(state);
		}
	}
}

void clock_int_logic(state_t *state){
	bool state_is_changed = false;
	if(state->cycle_ticks == 0){
		state_is_changed = true;
		sonar_trigger_high(state);
	}else if(state->cycle_ticks == 1){
		state_is_changed = true;
		sonar_trigger_low(state);
	}
	
	if(state->io_ticks == 0){
		unsigned char previous_state = state->data;
		load(state);
		
		move_logic(state);
		
		state_is_changed |= previous_state ^ state->data;
	}
	
	if(state_is_changed){
		update_data(state);
	}
	
	cycle_inc(state);
	io_inc(state);
}

/**
 * Interrupt vectors
 */
/**
 * Potwierdzenie przyjecia przerwania
 */
void EOI(){
	asm {
		mov al, 20h
		out 20h, al
	}
}

#define DISABLE_INTERRUPTS asm cli
#define ENABLE_INTERRUPTS asm sti

/**
 * Funkcja zastępujące przerwanie zegarowe IRQ0
 */
void interrupt clock_tick(){
	DISABLE_INTERRUPTS
	
	clock_inc(&state);
	clock_int_logic(&state);
	
	if(state.clock_edge){
		clock_fall_edge(&state);
		primary_state.old_clock();
	}else{
		EOI();
	}
	ENABLE_INTERRUPTS
}

/**
 * Funkcja wykonywana przy przerwaniu IRQ7
 * wywolywanym przez linie ACK z portu LPT
 */
void interrupt sonar_end_signal_receive(){
	DISABLE_INTERRUPTS
	
	ack_int_logic(&state);
	
	EOI();
	ENABLE_INTERRUPTS
}


void init(state_t *state, primary_state_t *primary_state){
	DISABLE_INTERRUPTS

	init_state(state);
	init_clock(primary_state);
	init_ack(primary_state);
	
	ENABLE_INTERRUPTS
}

void init_state(state_t *state){
	send_port(LPT_DATA, 0);
	load(state);
	state->range = 4000;
	state->cycle_ticks = 0;
	state->io_ticks = 0;
	
	state->clock_ticks = 0;
	state->clock_thirds = 0;
	state->clock_edge = false;
	
}

/**
 * Zapamietanie starego wektora przerwania,
 * podstawienie nowego i przyspieszenie zegara.
 *
 * Do zegara w 8253 dochodzi czestotliwosc 1193180Hz.
 * Wysylajac slowo sterujace do tego ukladu mozna
 * podzielic ta czestotliwosc:
 *
 * 1193180Hz / 12 = 99431,(6)Hz (ok. 10,057158 mikrosekund)
 * Taka czestotliwosc jest wymuszona przez koniecznosc
 * sterowania sonarem.
 */
void init_clock(primary_state_t *primary_state){
	primary_state->old_clock = getvect(8);
	setvect(8, clock_tick);
	
	asm {
			mov bx, 12
			mov al, 00110100b
			out 43h, al
			mov al, bl
			out 40h, al
			mov al, bh
			out 40h, al
	}
}

void init_ack(primary_state_t *primary_state){
	primary_state->old_ack_int = getvect(0x0f);
	setvect(0x0f, sonar_end_signal_receive);

	primary_state->old_lpt_ctrl = receive_port(LPT_CONTROL);
	send_port(LPT_CONTROL, LIGHT(primary_state->old_lpt_ctrl, PIN(4)));

	primary_state->old_8259_state = receive_port(0x21);
	send_port(0x21, DARK(primary_state->old_8259_state, PIN(7)));
}



void restore(primary_state_t *primary_state){
	DISABLE_INTERRUPTS

	restore_clock(primary_state);
	restore_ack(primary_state);
	send_port(LPT_DATA, 0);
	
	ENABLE_INTERRUPTS
}
/**
 * xor bx, bx    ; To ustawia 0 w bx  (a xor a == 0)
 *
 * Domyslny dzielnik czestotliwosci to 65536 (2^16),
 * ale dzielnik zajmuje tylko 2 bajty, wiec 2^16 sie
 * nie miesci w nim i jest to kodowane przez 0
 */
void restore_clock(primary_state_t *primary_state){
	setvect(8, primary_state->old_clock);
	
	asm {
		xor bx, bx
		mov al, 00110110b
		out 43h, al
		mov al, bl
		out 40h, al
		mov al, bh
		out 40h, al
	}
}

void restore_ack(primary_state_t *primary_state){
	setvect(0x0f, primary_state->old_ack_int);
	send_port(LPT_CONTROL, primary_state->old_lpt_ctrl);
	send_port(0x21, primary_state->old_8259_state);
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
	
	printf("\nRANGE: %lf cm\n\n", state->range);
}

int main(){
	bool still_work = true;
	char buf[512];
	
	init(&state, &primary_state);
	
	
	while(still_work){
		scanf(" %s", buf);
		
		if(strncmp(buf, "exit", 4) == 0){
			still_work = false;
		}else if(strncmp(buf, "state", 5) == 0){
			print_state(&state);
		}
		
	}
	
	
	restore(&primary_state);
	
	return 0;
}
