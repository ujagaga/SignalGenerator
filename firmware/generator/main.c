
/* Name: main.c
 * Project: AVR Pulse Generator
 *              for ATtiny2313
 * Author: Rada Berar
 * Creation Date: 30.11.2016.
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

#define USART_BAUDRATE 	(9600)
#define BAUD_PRESCALE 	(((( F_CPU / 16) + ( USART_BAUDRATE / 2) ) / ( USART_BAUDRATE ) ) - 1)
#define waitTxReady()	while (( UCSRA & (1 << UDRE ) ) == 0)
#define RX_SIZE 		(6)

#define CMD_SET_LEN		(5)
#define CMD_STORE_LEN	(1)
#define OUT_VALUE		(0xFF)

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
volatile uint32_t tmrCounter;
volatile uint8_t outState;

void check_command( void ){

	uint8_t command = rx_buf[0];

	if((rx_index == CMD_STORE_LEN) && (command == CMD_STORE)){

		eeprom_write_dword(&storePauseLen, pauseLen);
		eeprom_write_dword(&storePulseLen, pulseLen);
		rx_index = 0;

	}else if(rx_index == CMD_SET_LEN){

		uint32_t value = ((uint32_t)rx_buf[1] << 24) | ((uint32_t)rx_buf[2] << 16) | ((uint32_t)rx_buf[3] << 8) | rx_buf[4];

		if(value > 0){
			value -= 1;
		}

		if(command == CMD_SET_PAUSE){
			pauseLen = value;
		}else if(command == CMD_SET_PULSE){
			pulseLen = value;
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

ISR(TIMER0_COMPA_vect)
{
	tmrCounter++;

//	if(outState == 0){
//		if(tmrCounter > pauseLen){
//			outState = OUT_VALUE;
//			PORTB = OUT_VALUE;
//			tmrCounter = 0;
//		}
//	}else{
//		if(tmrCounter > pulseLen){
//			outState = 0;
//			PORTB = 0;
//			tmrCounter = 0;
//		}
//	}

	if(PORTB == 0){
		if(tmrCounter > pauseLen){
			PORTB = OUT_VALUE;
			tmrCounter = 0;
		}
	}else{
		if(tmrCounter > pulseLen){
			PORTB = 0;
			tmrCounter = 0;
		}
	}
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

	/* timer_init */
	OCR0A  = 200;      						// number to count up to.
	TIMSK |= (1 << OCIE0A);					// Set interrupt on compare match
	TCCR0A |= (1 << WGM01); 				// Clear Timer on Compare Match (CTC) mode
	TCCR0B = (1<<CS00);   	 				// no prescaling


	rx_index = 0;
	pauseLen = eeprom_read_dword(&storePauseLen);
	pulseLen = eeprom_read_dword(&storePulseLen);

	if(pauseLen == 0xFFFFFFFF){
		pauseLen = 0;
	}

	if(pulseLen == 0xFFFFFFFF){
		pulseLen = 0;
	}

	tmrCounter = 0;
	outState = 0;

	sei();
}


int main(void)
{
	init();

    for(;;){    /* main event loop */
        wdt_reset();


    }
    return 0;
}
