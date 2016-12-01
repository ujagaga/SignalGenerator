
/* Name: main.c
 * Project: AVR Pulse Generator
 *              for ATtiny2313
 * Author: Rada Berar
 * Creation Date: 30.11.2016.
 *
 * commands:
 * 00 <4 byte duration in 0,1us>	set pause length
 * 01 <4 byte duration in 0,1us>	set pulse length
 * 02								store settings to EEPROM and use at next startup
 *
 */

#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define nop() 			do{ __asm__ __volatile__ ("nop"); } while (0)

#define TMR0_MAX_COUNT		(250)
#define MIN_COUNT_MODE_LEN	(50)

#define USART_BAUDRATE 	(9600)
#define BAUD_PRESCALE 	(((( F_CPU / 16) + ( USART_BAUDRATE / 2) ) / ( USART_BAUDRATE ) ) - 1)
#define waitTxReady()	while (( UCSRA & (1 << UDRE ) ) == 0)
#define RX_SIZE 		(6)
#define CMD_SET_LEN		(5)
#define CMD_STORE_LEN	(1)
#define MAX_LEN			(0xFFFF >> 2)
#define SHORT_TIME_CORRECT	(2)
#define LONG_PAUSE_CORRECT	(26)
#define LONG_PULSE_CORRECT	(48)

#define OUT_SET()		do{ PORTB = 0xFF; }while(0)
#define OUT_CLR()		do{ PORTB = 0; }while(0)
#define OUT_TGL()		do{ PINB = 0xFF; }while(0)

#define START_TMR()		do{TCCR0B = (1 << CS00);}while(0) /* Start timer0 with no prescaller */
#define STOP_TMR()		do{TCCR0B = 0;}while(0)

enum{
	CMD_SET_PAUSE = 0,
	CMD_SET_PULSE = 1,
	CMD_STORE = 2, 		/* Save in EEPROM */
	CMD_UNKNOWN
};


uint32_t EEMEM storePauseLen = 1;
uint32_t EEMEM storePulseLen = 1;

volatile uint8_t rx_buf[RX_SIZE];
volatile uint8_t rx_index;
volatile uint32_t pauseLen;	/* pause duration in us */
volatile uint32_t pulseLen; /* pulse duration in us */
volatile bool modeChanged = false;
volatile uint32_t tmr0CycleCount;


static void check_command( void ){

	uint8_t command = rx_buf[0];

	if((rx_index == CMD_STORE_LEN) && (command == CMD_STORE)){

		eeprom_write_dword(&storePauseLen, pauseLen);
		eeprom_write_dword(&storePulseLen, pulseLen);
		rx_index = 0;

	}else if(rx_index == CMD_SET_LEN){

		uint32_t value = ((uint32_t)rx_buf[1] << 24) | ((uint32_t)rx_buf[2] << 16) | ((uint32_t)rx_buf[3] << 8) | rx_buf[4];
		if(value > MAX_LEN){
			value = MAX_LEN;
		}

		if(command == CMD_SET_PAUSE){
			pauseLen = value;
			modeChanged = true;
		}else if(command == CMD_SET_PULSE){
			pulseLen = value;
			modeChanged = true;
		}

		rx_index = 0;
	}
}


ISR(USART_RX_vect) {

	rx_buf[rx_index] = UDR;

	if(rx_index < RX_SIZE){
		rx_index++;
	}

	check_command();
}


static void init(void)
{
	ACSR |= 1 << ACD; /* Disable analog comparer to reduce power consumption */

	DDRB = 0xff;
	PORTB = 0;

	/* UART init */
	// set the baud rate
	UBRRL = (unsigned char)BAUD_PRESCALE;
	UBRRH = (BAUD_PRESCALE >> 8);
	// enable rx
	UCSRB = (1<<RXEN);
	//  enable RX interrupt
	UCSRB |= (1 << RXCIE);


	rx_index = 0;
	pauseLen = eeprom_read_dword(&storePauseLen);
	pulseLen = eeprom_read_dword(&storePulseLen);

	if(pauseLen == 0xFFFFFFFF){
		pauseLen = 10;
	}

	if(pulseLen == 0xFFFFFFFF){
		pulseLen = 10;
	}

	sei();
}

void TGL_01us( void ){
	while(!modeChanged){
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
		OUT_TGL();
	}
}

void TGL_02us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
		nop();
		OUT_TGL();
	}
}

void TGL_03us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		OUT_TGL();
	}
}

void TGL_04us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		OUT_TGL();


	}
}

void TGL_05us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		OUT_TGL();
	}
}

void TGL_06us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		nop();
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		nop();
		OUT_TGL();

	}
}

void TGL_07us( void ){
	while(!modeChanged){
		OUT_TGL();
	}
}

void TGL_08us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
	}
}

void TGL_09us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
	}
}

void TGL_1us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
	}
}

void TGL_12us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		nop();
	}
}


void TGL_14us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
	}
}


void TGL_16us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
	}
}


void TGL_18us( void ){
	while(!modeChanged){
		OUT_TGL();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
		nop();
	}
}


void doToggle( void ){

	modeChanged = false;

	uint8_t period = pauseLen + pulseLen;

	switch(period){
	case 1:
		TGL_01us();
		break;
	case 2:
		TGL_02us();
		break;
	case 3:
		TGL_03us();
		break;
	case 4:
		TGL_04us();
		break;
	case 5:
		TGL_05us();
		break;
	case 6:
		TGL_06us();
		break;
	case 7:
		TGL_07us();
		break;
	case 8:
		TGL_08us();
		break;
	case 9:
		TGL_09us();
		break;
	case 10:
		TGL_1us();
		break;
	case 11:
	case 12:
		TGL_12us();
		break;
	case 13:
	case 14:
		TGL_14us();
		break;
	case 15:
	case 16:
		TGL_16us();
		break;
	default:
		TGL_18us();
		break;
		break;
	}

}


ISR (TIMER0_COMPA_vect)
{
	tmr0CycleCount++;
}


void doCounting( void ){
	OCR0A  = TMR0_MAX_COUNT - 1;// number to count up to.
	TCCR0A = (1 << WGM01); 		// CTC mode
	TIMSK |= (1 << OCIE0A);		// Set interrupt on compare match


	uint32_t tmrLowCycleCount = pauseLen / TMR0_MAX_COUNT;
	uint32_t tmrTotalCycleCount = (pauseLen + pulseLen) / TMR0_MAX_COUNT;
	uint8_t tmrLowRemanant = (pauseLen % TMR0_MAX_COUNT);
	uint8_t tmrTotalRemanant = (pauseLen + pulseLen) % TMR0_MAX_COUNT;

	/* Make corrections to account for flow controll */
	if(tmrLowRemanant > LONG_PAUSE_CORRECT){
		tmrLowRemanant -= LONG_PAUSE_CORRECT;
	}else{
		if(tmrLowCycleCount > 0){
			tmrLowCycleCount --;
			tmrLowRemanant = TMR0_MAX_COUNT - 1 + tmrLowRemanant - LONG_PAUSE_CORRECT;
		}
	}

	/* Make corrections to account for flow controll */
	if(tmrTotalRemanant > LONG_PULSE_CORRECT){
		tmrTotalRemanant -= LONG_PULSE_CORRECT;
	}else{
		if(tmrTotalCycleCount > 0){
			tmrTotalCycleCount --;
			tmrTotalRemanant = TMR0_MAX_COUNT - 1 + tmrTotalRemanant - LONG_PULSE_CORRECT;
		}
	}

	modeChanged = false;

	TCNT0 = 0;
	tmr0CycleCount = 0;

	START_TMR();

	while(!modeChanged){
		while(tmr0CycleCount < tmrLowCycleCount);
		while(TCNT0 < tmrLowRemanant);
		OUT_SET();
		while(tmr0CycleCount < tmrTotalCycleCount);
		while(TCNT0 < tmrTotalRemanant);
		OUT_CLR();
		TCNT0 = 0;
		tmr0CycleCount = 0;
	}

	STOP_TMR();


}


int main(void)
{
	init();

    for(;;){    /* main event loop */
    	if((pauseLen < 19) && (pulseLen < 19) && ((pauseLen + pulseLen) < 19)){
    		/* PWM mode */
    		doToggle();
    	}else{
    		/* count mode */
    		doCounting();
    	}
    }
    return 0;
}

