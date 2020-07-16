/* TODO
 * LCD functions for hand onctroller
 * E-stop appears to be normal stop


 /* DCC/MM2 CENTRAL BY SEBASTIAAN KNIPPELS
	*	this program lets the arduino act as a conduit between train controllers and the DCC/MM2 track signal
	*	The arduino simply outputs digital signals on the track and it accepts instructions serially from a hand controller
	*	The arduino stores in EEPROM memory of which decoder Types the decoders are. A decoder may be DCC14, DCC28, MM2 or it is empty
	*	when it is empty no DCC/MM2 packages for that addres will be outputted
	*/


#define true 1
#define false 0
//#include <EEPROM.h>
//#include "config.h"
#include "DCC.h"
#include "Serial.h"
#include "Arduino.h"
#include "src/basics/io.h"
#include "src/basics/timers.h"



/***** ROUND ROBIN TASKS ******/
void EstopPressed() {
	digitalWrite(power_on, LOW);
	Serial.print("$$80");
	Serial.println("E-stop pressed"); } 

#define MAXIMUM_CURRENT 160
void shortCircuit() {
	static byte msCounter = 0;
	static unsigned int prev;

	if(!overloadT) {
		overloadT = 10;
	

		unsigned int sample = analogRead(currentSensePin);
		if(sample < MAXIMUM_CURRENT) {
			msCounter = 5;
		}/*
		if(sample != prev) {
			prev = sample;
			Serial.println(sample);
		}*/
		
		if(msCounter) msCounter--;
		if(!msCounter) EstopPressed();
	}
}

/***** INITIALIZATION *****/
void setup() {
	Serial.begin(115200);
	Serial.println("DCC CENTRAL V1.0");


	initIO();
	digitalWrite(power_on,LOW); // must be low
	digitalWrite(directionPin2, HIGH);
	digitalWrite(directionPin, LOW);

	initTimers();
	initDCC();
	connectT = 10;
}

/***** MAIN LOOP *****/
void loop() {
	connect();

	readSerialBus();

	shortCircuit();

	DCCsignals();
}



