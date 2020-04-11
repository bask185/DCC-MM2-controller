#include <Arduino.h>
#include "io.h"
extern void initIO(void) {
	pinMode(directionPin2, OUTPUT);
	pinMode(directionPin, OUTPUT);
	pinMode(power_on, OUTPUT);
	pinMode(blinkLed, OUTPUT);
	pinMode(currentSensePin, INPUT);
}