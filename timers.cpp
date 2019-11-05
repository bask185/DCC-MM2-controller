#include <Arduino.h>
#include "timers.h"

unsigned char makeNumberT = 0;
unsigned char  overloadT = 0;

void updateTimers() {		// take note, this function has deviating indentations
	static byte toggle = 0, ms, cs;

	if((millis() & 0x1) ^ toggle) {	toggle ^= 1; ms++; // every ms
		if(overloadT) overloadT--;

	if(ms == 10) {	ms = 0;	cs += 1;					// every 10ms
		if(makeNumberT) makeNumberT--;

	if(cs == 10) {	cs = 0; 							// every 100ms

} } } }