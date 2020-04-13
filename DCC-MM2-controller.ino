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
#include <EEPROM.h>
#include "config.h"
#include "DCC.h"
#include "Serial.h"
#include "Arduino.h"
#include "src/basics/io.h"
#include "src/basics/timers.h"


#define printDecoder(x) case x: Serial.println(#x); break;
void getDecoderTypes() {
	int j;
	//Serial.println("The following trains are listed");
	for(j=1;j<=80;j++) {
		train[j].decoderType = EEPROM.read(j); 
		//Serial.println(j);
		if(train[j].decoderType < 3) {
			//Serial.print("Addres "); Serial.print(j); Serial.print(" type ");
			/*switch(train[j].decoderType) {
				default: Serial.println("empty");break;
				printDecoder(MM2);
				printDecoder(DCC14);
				printDecoder(DCC28); }*/ } }
	
	train[8].decoderType = DCC28; 
	train[8].speed = 40;
	train[8].headLight = 1;
	}

/***** ROUND ROBIN TASKS ******/
void EstopPressed() {
	//digitalWrite(power_on, LOW);
	Serial.print("$$80");
	/*Serial.println("E-stop pressed");*/ } 

void shortCircuit() {
	if(analogRead(currentSensePin) < MAXIMUM_CURRENT) {
		overloadT = 50; }
	if(!overloadT) EstopPressed();
}

/***** INITIALIZATION *****/
void setup() {
	Serial.begin(115200);
	//timeOutT = 255;
	//while(!Serial);
	//Serial.begin(115200);
	//while(!Serial);
	
	//Serial.println("DCC centrale v1.0");

	delay(500);
	// CLEAR_PHONE 

	for(int i=1;i<=80;i++) train[i].speed = 28;
	getDecoderTypes();

	initIO();
	digitalWrite(power_on,LOW); // must be low
	digitalWrite(directionPin2, HIGH);
	digitalWrite(directionPin, LOW);

	initTimers();
	initDCC();
	//Serial.print("%%"); // starts debug mode
}

/***** MAIN LOOP *****/
void loop() {

	readSerialBus();														
	shortCircuit();

	DCCsignals();
}


// void print_binary(byte var) {
// 	for (unsigned int test = 0x80; test; test >>= 1) {
// 		Serial.write(var	& test ? '1' : '0'); }
// 	Serial.write(' ');}
