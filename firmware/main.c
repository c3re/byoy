/* Name: main.c
 * Project: labortage-2013-badge
 * Author: bg (bg@das-labor.org)
 * Creation Date: 2013-10-16
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH, (c) Daniel Otte
 * License: GNU GPL v3
 * This Revision: $Id: main.c 692 2008-11-07 15:07:40Z cs $
 */

/*
This example should run on most AVRs with only little changes. No special
hardware resources except INT0 are used. You may have to change usbconfig.h for
different I/O pins for USB. Please note that USB D+ must be the INT0 pin, or
at least be connected to INT0 as well.
*/

#define BUTTON_PIN 0
#define DEBOUNCE_DELAY 50
#define SIMPLE_COUNTER 1
#define NO_CHECK 1
#define ALLOW_SECRET_READ 0
#define SEND_NUM_TOKENS 3   /* how many tokens to send on startup */
#define STARTUP_DELAY 170000 /* in main-loop iterations */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"
#include "requests.h"       /* The custom request numbers we use */
#include "hotp.h"
#include "special_functions.h"
#if !SIMPLE_COUNTER
#include "percnt2.h"
#endif
#include "usb_keyboard_codes.h"

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

#define STATE_WAIT 0
#define STATE_SEND_KEY 1
#define STATE_RELEASE_KEY 2
#define STATE_NEXT 3

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                    /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06,                    /* USAGE (Keyboard) */
    0xa1, 0x01,                    /* COLLECTION (Application) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x95, 0x08,                    /*   REPORT_COUNT (8) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard)(Key Codes) */
    0x19, 0xe0,                    /*   USAGE_MINIMUM (Keyboard LeftControl)(224) */
    0x29, 0xe7,                    /*   USAGE_MAXIMUM (Keyboard Right GUI)(231) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01,                    /*   LOGICAL_MAXIMUM (1) */
    0x81, 0x02,                    /*   INPUT (Data,Var,Abs) ; Modifier byte */
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x81, 0x03,                    /*   INPUT (Cnst,Var,Abs) ; Reserved byte */
    0x95, 0x05,                    /*   REPORT_COUNT (5) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x05, 0x08,                    /*   USAGE_PAGE (LEDs) */
    0x19, 0x01,                    /*   USAGE_MINIMUM (Num Lock) */
    0x29, 0x05,                    /*   USAGE_MAXIMUM (Kana) */
    0x91, 0x02,                    /*   OUTPUT (Data,Var,Abs) ; LED report */
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x03,                    /*   REPORT_SIZE (3) */
    0x91, 0x03,                    /*   OUTPUT (Cnst,Var,Abs) ; LED report padding */
    0x95, 0x06,                    /*   REPORT_COUNT (6) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x65,                    /*   LOGICAL_MAXIMUM (101) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard)(Key Codes) */
    0x19, 0x00,                    /*   USAGE_MINIMUM (Reserved (no event indicated))(0) */
    0x29, 0x65,                    /*   USAGE_MAXIMUM (Keyboard Application)(101) */
    0x81, 0x00,                    /*   INPUT (Data,Ary,Abs) */
    0xc0                           /* END_COLLECTION */
};

static uint16_t secret_length_ee EEMEM = 0;
static uint8_t  secret_ee[32] EEMEM;
static uint8_t  reset_counter_ee EEMEM = 0;
static uint8_t  digits_ee EEMEM = 8;

#if SIMPLE_COUNTER
static uint32_t counter_ee EEMEM = 0;
#endif

static uint8_t dbg_buffer[8];
static uint8_t secret[32];
static uint16_t secret_length_b;
static char token[10];

#define UNI_BUFFER_SIZE 16

static union __attribute__((packed)) {
	uint8_t  w8[UNI_BUFFER_SIZE];
	uint16_t w16[UNI_BUFFER_SIZE/2];
	uint32_t w32[UNI_BUFFER_SIZE/4];
	void*    ptr[UNI_BUFFER_SIZE/sizeof(void*)];
} uni_buffer;

static uint8_t current_command;

typedef struct __attribute__((packed)) {
    uint8_t modifier;
    uint8_t reserved;
    uint8_t keycode[6];
} keyboard_report_t;

static keyboard_report_t keyboard_report; /* report sent to the host */
static uchar idleRate;  /* in 4 ms units */
static uchar key_state = STATE_WAIT;
volatile static uchar LED_state = 0xff;
/* ------------------------------------------------------------------------- */

static
void memory_clean(void) {
    memset(secret, 0, 32);
    secret_length_b = 0;
}

static
uint8_t secret_set(void){
#if !NO_CHECK
    uint8_t r;
    union {
        uint8_t w8[32];
        uint16_t w16[16];
    } read_back;
#endif
    const uint8_t length_B = (secret_length_b + 7) / 8;

    eeprom_busy_wait();
    eeprom_write_block(secret, secret_ee, length_B);
#if !NO_CHECK
    eeprom_busy_wait();
    eeprom_read_block(read_back.w8, secret_ee, length_B);
    r = memcmp(secret, read_back.w8, length_B);
    memory_clean();
    memset(read_back.w8, 0, 32);
    if (r) {
        return 1;
    }
#endif
    eeprom_busy_wait();
    eeprom_write_word(&secret_length_ee, secret_length_b);
#if !NO_CHECK
    eeprom_busy_wait();
    r = eeprom_read_word(&secret_length_ee) == secret_length_b;
    memory_clean();
    *read_back.w16 = 0;
    if (!r) {
        return 1;
    }
#else
    memory_clean();
#endif

    return 0;
}

static
void counter_inc(void){
#if SIMPLE_COUNTER
    uint32_t t;
    eeprom_busy_wait();
    t = eeprom_read_dword(&counter_ee);
    eeprom_busy_wait();
    eeprom_write_dword(&counter_ee, t + 1);
#else
    percnt_inc(0);
#endif
}

static
void counter_reset(void) {
    uint8_t reset_counter;
    eeprom_busy_wait();
    reset_counter = eeprom_read_byte(&reset_counter_ee);
#if SIMPLE_COUNTER
    eeprom_busy_wait();
    eeprom_write_dword(&counter_ee, 0);
#else
    percnt_reset(0);
#endif
    eeprom_busy_wait();
    eeprom_write_byte(&reset_counter_ee, reset_counter + 1);
}

static
void counter_init(void) {
#if !SIMPLE_COUNTER
    eeprom_busy_wait();
    if (eeprom_read_byte(&reset_counter_ee) == 0) {
        counter_reset();
    }
    percnt_init(0);
#endif
}

static
void token_generate(void) {
    uint16_t s_length_b;
    uint8_t digits;
    counter_inc();
    eeprom_busy_wait();
    eeprom_read_block(secret, secret_ee, 32);
    eeprom_busy_wait();
    s_length_b = eeprom_read_word(&secret_length_ee);
    if (s_length_b > 256) {
        s_length_b = 256;
    }
    eeprom_busy_wait();
    digits = eeprom_read_byte(&digits_ee);
#if SIMPLE_COUNTER
    eeprom_busy_wait();
    hotp(token, secret, s_length_b, eeprom_read_dword(&counter_ee) - 1, digits);
#else
    hotp(token, secret, s_length_b, percnt_get(0) - 1, digits);
#endif
    memory_clean();
}


static
void buildReport(uchar send_key) {
    keyboard_report.modifier = 0;
    switch (send_key) {
    case '1' ... '9':
        keyboard_report.keycode[0] = KEY_1 + (send_key-'1');
        break;
    case '0':
        keyboard_report.keycode[0] = KEY_0;
        break;
    default:
        keyboard_report.keycode[0] = 0;
    }
}

static
int8_t button_get_debounced(volatile int8_t debounce_count) {
    uint8_t v;
    v = PINB & _BV(BUTTON_PIN);
    while (debounce_count-- && (v == (PINB & _BV(BUTTON_PIN)))) {
        ;
    }
    if (debounce_count != -1) {
        return -1;
    }
    return v ? 0 : 1;
}

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (usbRequest_t *)data;
	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {    /* class request type */
	    switch(rq->bRequest) {
        case USBRQ_HID_GET_REPORT: /* send "no keys pressed" if asked here */
            /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            usbMsgPtr = (void *)&keyboard_report; /* we only have this one */
            keyboard_report.modifier = 0;
            keyboard_report.keycode[0] = 0;
            return sizeof(keyboard_report);
        case USBRQ_HID_SET_REPORT: /* if wLength == 1, should be LED state */
            if (rq->wLength.word == 1) {
                current_command = LED_WRITE;
                return USB_NO_MSG;
            }
            return 0;
        case USBRQ_HID_GET_IDLE: /* send idle rate to PC as required by spec */
            usbMsgPtr = &idleRate;
            return 1;
        case USBRQ_HID_SET_IDLE: /* save idle rate as required by spec */
            idleRate = rq->wValue.bytes[1];
            return 0;
        }
    }
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR) {
		current_command = rq->bRequest;
        usbMsgPtr = uni_buffer.w8;
    	switch(rq->bRequest)
		{
    	case CUSTOM_RQ_SET_SECRET:
    	    secret_length_b = rq->wValue.word;
    	    if (secret_length_b > 256) {
    	        secret_length_b = 256;
    	    }
    	    uni_buffer.w8[0] = 0;
    	    return USB_NO_MSG;
    	case CUSTOM_RQ_INC_COUNTER:
    	    counter_inc();
    	    return 0;
    	case CUSTOM_RQ_GET_COUNTER:
#if SIMPLE_COUNTER
    	    eeprom_busy_wait();
    	    uni_buffer.w32[0] = eeprom_read_dword(&counter_ee);
#else
    	    uni_buffer.w32[0] = percnt_get(0);
#endif
    	    return 4;
    	case CUSTOM_RQ_RESET_COUNTER:
    	    counter_reset();
    	    return 0;
        case CUSTOM_RQ_GET_RESET_COUNTER:
    	    eeprom_busy_wait();
            uni_buffer.w8[0] = eeprom_read_byte(&reset_counter_ee);
            return 1;
    	case CUSTOM_RQ_SET_DIGITS:
            if (rq->wValue.bytes[0] < 6) {
                rq->wValue.bytes[0] = 6;
            }
    	    if (rq->wValue.bytes[0] > 9) {
    	        rq->wValue.bytes[0] = 9;
    	    }
    	    eeprom_busy_wait();
    	    eeprom_write_byte(&digits_ee, rq->wValue.bytes[0]);
    	    return 0;
    	case CUSTOM_RQ_GET_DIGITS:
    	    eeprom_busy_wait();
            uni_buffer.w8[0] = eeprom_read_byte(&digits_ee);
            return 1;
    	case CUSTOM_RQ_GET_TOKEN:
    	    token_generate();
    	    usbMsgPtr = (usbMsgPtr_t)token;
    	    return strlen(token);
    	case CUSTOM_RQ_PRESS_BUTTON:
    	    key_state = STATE_SEND_KEY;
    	    return 0;
    	case CUSTOM_RQ_CLR_DBG:
            memset(dbg_buffer, 0, sizeof(dbg_buffer));
            return 0;
		case CUSTOM_RQ_SET_DBG:
			return USB_NO_MSG;
		case CUSTOM_RQ_GET_DBG:
			usbMsgPtr = dbg_buffer;
			return MIN(8, rq->wLength.word);
		case CUSTOM_RQ_RESET:
			soft_reset((uint8_t)(rq->wValue.word));
			break;
		case CUSTOM_RQ_READ_BUTTON:
			uni_buffer.w8[0] = button_get_debounced(DEBOUNCE_DELAY);
			return 1;
#if ALLOW_SECRET_READ
		case CUSTOM_RQ_GET_SECRET:
            uni_buffer.w8[0] = 0;
		    return USB_NO_MSG;
#else
#endif
        default:
            return 0;
		}
    }

    return 0;   /* default for not implemented requests: return no data back to host */
}


uchar usbFunctionWrite(uchar *data, uchar len)
{
	switch(current_command){

	case LED_WRITE:
	    if (data[0] != LED_state)
	        LED_state = data[0];
	    return 1; /* Data read, not expecting more */
	case CUSTOM_RQ_SET_SECRET:
        {
            if (uni_buffer.w8[0] < (secret_length_b + 7) / 8) {
                memcpy(&secret[uni_buffer.w8[0]], data, len);
                uni_buffer.w8[0] += len;
            }
            if (uni_buffer.w8[0] >= (secret_length_b + 7) / 8) {
                secret_set();
                return 1;
            }
            return 0;
        }
	case CUSTOM_RQ_SET_DBG:
		if(len > sizeof(dbg_buffer)){
			len = sizeof(dbg_buffer);
		}
		memcpy(dbg_buffer, data, len);
	}
	return 1;
}

uchar usbFunctionRead(uchar *data, uchar len){
#if ALLOW_SECRET_READ || 1
    uchar r;
    uint8_t s_length_B;
    switch(current_command){
    case CUSTOM_RQ_GET_SECRET:
        eeprom_busy_wait();
        s_length_B = (eeprom_read_word(&secret_length_ee) + 7) / 8;
        r = MIN(len, s_length_B - uni_buffer.w8[0]);
        eeprom_busy_wait();
        eeprom_read_block(data, secret_ee + uni_buffer.w8[0], r);
        uni_buffer.w8[0] += r;
        return r;
    }
#endif
    return 0;
}

static void calibrateOscillator(void)
{
uchar       step = 128;
uchar       trialValue = 0, optimumValue;
int         x, optimumDev, targetValue = (unsigned)(1499 * (double)F_CPU / 10.5e6 + 0.5);
 
    /* do a binary search: */
    do {
        OSCCAL = trialValue + step;
        x = usbMeasureFrameLength();    /* proportional to current real frequency */
        if(x < targetValue)             /* frequency still too low */
            trialValue += step;
        step >>= 1;
    } while(step > 0);
    /* We have a precision of +/- 1 for optimum OSCCAL here */
    /* now do a neighborhood search for optimum value */
    optimumValue = trialValue;
    optimumDev = x; /* this is certainly far away from optimum */
    for (OSCCAL = trialValue - 1; OSCCAL <= trialValue + 1; OSCCAL++){
        x = usbMeasureFrameLength() - targetValue;
        if (x < 0)
            x = -x;
        if (x < optimumDev) {
            optimumDev = x;
            optimumValue = OSCCAL;
        }
    }
    OSCCAL = optimumValue;
}
 

void usbEventResetReady(void)
{
    cli();  /* usbMeasureFrameLength() counts CPU cycles, so disable interrupts. */
    calibrateOscillator();
    sei();
}

/* ------------------------------------------------------------------------- */

int main(void)
{
	size_t idx = 0;
	int8_t i = 0;

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */

    counter_init();
    usbInit();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    while(--i){             /* fake USB disconnect for ~512 ms */
        wdt_reset();
        _delay_ms(2);
    }
    usbDeviceConnect();
	
    sei();
    DDRB &= ~_BV(BUTTON_PIN); /* make button pin input */
    PORTB |= _BV(BUTTON_PIN); /* turn on pull-up resistor */

    int t = 0;
    uint32_t w = 0;            /* start delay */
    for(;;w++){                /* main event loop */
        wdt_reset();
        usbPoll();
        if(usbInterruptIsReady() && w > STARTUP_DELAY){
            if(key_state == STATE_WAIT && t < SEND_NUM_TOKENS) {
              token_generate();
              key_state = STATE_SEND_KEY;
              t++;
            }
            switch(key_state) {
            case STATE_SEND_KEY:
                buildReport(token[idx]);
                key_state = STATE_RELEASE_KEY; /* release next */
                break;
            case STATE_RELEASE_KEY:
                buildReport(0);
                ++idx;
                if (token[idx] == '\0') {
                    idx = 0;
                    key_state = STATE_WAIT;
                } else {
                    key_state = STATE_SEND_KEY;
                }
                break;
            default:
                key_state = STATE_WAIT; /* should not happen */
            }
                        /* start sending */
            usbSetInterrupt((void *)&keyboard_report, sizeof(keyboard_report));

        }

    }
    return 0;
}

/* ------------------------------------------------------------------------- */
