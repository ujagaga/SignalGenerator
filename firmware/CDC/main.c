
/* Name: main.c
 * Project: AVR USB driver for CDC interface on Low-Speed USB
 *              for ATtiny2313
 * Author: Osamu Tamura
 * Creation Date: 2007-10-03
 * Tabsize: 4
 * Copyright: (c) 2007-2009 by Recursion Co., Ltd.
 * License: Proprietary, free under certain conditions. See Documentation.
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
#include "usbdrv/usbdrv.h"


#define HW_CDC_BULK_OUT_SIZE     8
#define HW_CDC_BULK_IN_SIZE      8
#define TX_SIZE        			(HW_CDC_BULK_OUT_SIZE)
#define USART_BAUDRATE 	(9600)
#define BAUD_PRESCALE 	(((( F_CPU / 16) + ( USART_BAUDRATE / 2) ) / ( USART_BAUDRATE ) ) - 1)
#define waitTxReady()	while (( UCSRA & (1 << UDRE ) ) == 0)

enum {
    SEND_ENCAPSULATED_COMMAND = 0,
    GET_ENCAPSULATED_RESPONSE,
    SET_COMM_FEATURE,
    GET_COMM_FEATURE,
    CLEAR_COMM_FEATURE,
    SET_LINE_CODING = 0x20,
    GET_LINE_CODING,
    SET_CONTROL_LINE_STATE,
    SEND_BREAK
};

enum{
	CMD_SET_PAUSE = 0,
	CMD_SET_PULSE = 1,
	CMD_STORE = 2 		/* Save in EEPROM */
};



static uint8_t help_msg_buf[TX_SIZE];
static uint8_t help_msg_len;
static uint8_t txMsg;
static uint8_t txReadyFlag = 0;

const PROGMEM char configDescrCDC[] = {   /* USB configuration descriptor */
    9,          /* sizeof(usbDescrConfig): length of descriptor in bytes */
    USBDESCR_CONFIG,    /* descriptor type */
    67,
    0,          /* total length of data returned (including inlined descriptors) */
    2,          /* number of interfaces in this configuration */
    1,          /* index of this configuration */
    0,          /* configuration name string index */
#if USB_CFG_IS_SELF_POWERED
    (1 << 7) | USBATTR_SELFPOWER,       /* attributes */
#else
    (1 << 7),                           /* attributes */
#endif
    USB_CFG_MAX_BUS_POWER/2,            /* max USB current in 2mA units */

    /* interface descriptor follows inline: */
    9,          /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE, /* descriptor type */
    0,          /* index of this interface */
    0,          /* alternate setting for this interface */
    USB_CFG_HAVE_INTRIN_ENDPOINT,   /* endpoints excl 0: number of endpoint descriptors to follow */
    USB_CFG_INTERFACE_CLASS,
    USB_CFG_INTERFACE_SUBCLASS,
    USB_CFG_INTERFACE_PROTOCOL,
    0,          /* string index for interface */

    /* CDC Class-Specific descriptor */
    5,           /* sizeof(usbDescrCDC_HeaderFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    0,           /* header functional descriptor */
    0x10, 0x01,

    4,           /* sizeof(usbDescrCDC_AcmFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    2,           /* abstract control management functional descriptor */
    0x02,        /* SET_LINE_CODING,    GET_LINE_CODING, SET_CONTROL_LINE_STATE    */

    5,           /* sizeof(usbDescrCDC_UnionFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    6,           /* union functional descriptor */
    0,           /* CDC_COMM_INTF_ID */
    1,           /* CDC_DATA_INTF_ID */

    5,           /* sizeof(usbDescrCDC_CallMgtFn): length of descriptor in bytes */
    0x24,        /* descriptor type */
    1,           /* call management functional descriptor */
    3,           /* allow management on data interface, handles call management by itself */
    1,           /* CDC_DATA_INTF_ID */

    /* Endpoint Descriptor */
    7,           /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x83,        /* IN endpoint number 3 */
    0x03,        /* attrib: Interrupt endpoint */
    8, 0,        /* maximum packet size */
    USB_CFG_INTR_POLL_INTERVAL,        /* in ms */

    /* Interface Descriptor  */
    9,           /* sizeof(usbDescrInterface): length of descriptor in bytes */
    USBDESCR_INTERFACE,           /* descriptor type */
    1,           /* index of this interface */
    0,           /* alternate setting for this interface */
    2,           /* endpoints excl 0: number of endpoint descriptors to follow */
    0x0A,        /* Data Interface Class Codes */
    0,
    0,           /* Data Interface Class Protocol Codes */
    0,           /* string index for interface */

    /* Endpoint Descriptor */
    7,           /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x01,        /* OUT endpoint number 1 */
    0x02,        /* attrib: Bulk endpoint */
    HW_CDC_BULK_OUT_SIZE, 0,        /* maximum packet size */
    0,           /* in ms */

    /* Endpoint Descriptor */
    7,           /* sizeof(usbDescrEndpoint) */
    USBDESCR_ENDPOINT,  /* descriptor type = endpoint */
    0x81,        /* IN endpoint number 1 */
    0x02,        /* attrib: Bulk endpoint */
    HW_CDC_BULK_IN_SIZE, 0,        /* maximum packet size */
    0,           /* in ms */
};

const uint8_t EEMEM helpResponse[] = "help\tthis dialog\n"
		"FE, FF, 00, <4 byte value>\tpause ms\n"
		"FE, FF, 01, <4 byte value>\tpulse ms\n"
		"FE, FF, 02\tstores current settings";

uchar usbFunctionDescriptor(usbRequest_t *rq)
{

    if(rq->wValue.bytes[1] == USBDESCR_DEVICE){
        usbMsgPtr = (uchar *)usbDescriptorDevice;
        return usbDescriptorDevice[0];
    }else{  /* must be config descriptor */
        usbMsgPtr = (uchar *)configDescrCDC;
        return sizeof(configDescrCDC);
    }
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uchar    modeBuffer[7];

uchar usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */

        if( rq->bRequest==GET_LINE_CODING || rq->bRequest==SET_LINE_CODING ){
            return 0xff;
        /*    GET_LINE_CODING -> usbFunctionRead()    */
        /*    SET_LINE_CODING -> usbFunctionWrite()   */
        }
    }
    return 0;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionRead -  GET_LINE_CODING                                       */
/*---------------------------------------------------------------------------*/

uchar usbFunctionRead( uchar *data, uchar len )
{
    memcpy( data, modeBuffer, 7 );
    return 7;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionWrite  -  SET_LINE_CODING                                      */
/*---------------------------------------------------------------------------*/

uchar usbFunctionWrite( uchar *data, uchar len )
{
	// lets set the baud rate
	UBRRL = (unsigned char)BAUD_PRESCALE;
	UBRRH = (BAUD_PRESCALE >> 8);
    UCSRA   = (1<<U2X);
    UCSRB	= (1<<RXEN) | (1<<TXEN);

    memcpy( modeBuffer, data, 7 );
    return 1;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionWriteOut					                                     */
/*---------------------------------------------------------------------------*/






void usbFunctionWriteOut( uchar *data, uchar len )
{
	txMsg = 0;	/* Error response */
	txReadyFlag = 1;

	if(len > 2){
		uint8_t command = data[2];

		if(		(data[0] == 0xFF) && (data[1] != 0xFE) &&
				(((command < CMD_STORE) && (len > 6)) || ((command == CMD_STORE) && (len > 2))) ){
			uint8_t msg_len;

			if(command == CMD_STORE){
				msg_len = 3;
			}else{
				msg_len = 7;
			}

			/*  postpone receiving next data    */
			usbDisableAllRequests();

			uint8_t i;
			for(i=2; i<msg_len; i++){
				waitTxReady();
				UDR = data[i];
			}

			usbEnableAllRequests();

			txMsg = 1;	/* Acknowledge response */
		}else if((data[0] == 'h') && (data[1] != 'e') && (data[2] != 'l') && (data[1] != 'p')){
			help_msg_len = sizeof(helpResponse);
			txReadyFlag = 2;
		}
	}
}


static void hardwareInit(void)
{
unsigned	i;
uchar		j;

    /* activate pull-ups except on USB lines */
    USB_CFG_IOPORT   = (uchar)~((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));
    /* all pins input except USB (-> USB reset) */
    USBDDR    = (1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT);

    j = 15;
    while(--j){          /* USB Reset by device only required on Watchdog Reset */
        i = 0;
        while(--i)
            wdt_reset();
    }

    USBDDR    = 0;      /*  remove USB reset condition */

	PORTB	= 0xff;
}


int main(void)
{
    hardwareInit();
    usbInit();

    sei();
    for(;;){    /* main event loop */
        wdt_reset();
        usbPoll();

        /*    device => host     */
        if( usbInterruptIsReady()) {
        	if(txReadyFlag == 1){
        		usbSetInterrupt(&txMsg, 1);
        		txReadyFlag = 0;
        	}else if(txReadyFlag == 2){

        		uint8_t* nextData = &helpResponse[sizeof(helpResponse) - help_msg_len];
        		uint8_t txSize;

        		if(help_msg_len > TX_SIZE) {
        			help_msg_len -= TX_SIZE;
        			txSize = TX_SIZE;
        		}else{
        			txSize = help_msg_len;
        			txReadyFlag = 0;
        		}

        		eeprom_read_block(help_msg_buf, nextData, txSize);
        		usbSetInterrupt(help_msg_buf, txSize);
        	}

        }
    }
    return 0;
}
