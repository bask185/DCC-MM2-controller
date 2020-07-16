#include <Arduino.h>
#include "timers.h"

extern void initTimers() {
	TCCR2B = 0;// same for TCCR2B
	TCNT2  = 0;//initialize counter value to 0
	// set compare match register for 8khz increments
	OCR2A = 249;// = (16*10^6) / (1000*64) - 1 (must be <256)
	// turn on CTC mode
	TCCR2A |= (1 << WGM21);
	// Set CS21and and CS20 bit for 64 prescaler
	TCCR2B |= (1 << CS20);
	TCCR2B |= (1 << CS21); 
	TCCR2B &= ~(1 << CS22); 
	// enable timer compare interrupt
	TIMSK2 |= (1 << OCIE2A); }

volatile unsigned char overloadT;
volatile unsigned char makeNumberT;
volatile unsigned char handShakeT;
volatile unsigned char blinkT;
volatile unsigned char connectT;

// Don't complain about the indentations. This code is generated for you and you shouldn't be looking at this part.
ISR(TIMER2_COMPA_vect) {
static unsigned char _1ms, _10ms, _100ms;

// 1ms timers
_1ms += 1;

	if(overloadT) overloadT--;



// 10ms timers
if(!(_1ms % 10)) { _1ms = 0; _10ms += 1;

	if(makeNumberT) makeNumberT--;
	if(connectT) connectT--;



// 100ms timers
if(!(_10ms % 10)) { _10ms = 0; _100ms += 1;

	if(handShakeT) handShakeT--;
	if(blinkT) blinkT--;



//1000ms timers
if(!(_100ms % 10)) { _100ms = 0;




}
}
}
}


extern void repeat(byte *timer, byte interval, void (*function)()) {
	if(*timer == 0) {
		*timer = interval;
		function();
	}
}