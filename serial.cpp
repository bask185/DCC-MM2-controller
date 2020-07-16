#include "Serial.h"
#include "src/basics/timers.h"
#include "src/basics/io.h"
//#include "config.h"
#include <Arduino.h>
#include "DCC.h"
#include <EEPROM.h>

enum serialCommands { // 0x00 to 0x08 are the 8 primary commands. Each instruction will have followUp unsigned chars
	IDLE = 0,
	handControllerCommand = 'a',
	setDecoderType,
	processingCommand,
	trackCommand,
	newSpeed,
	settingFunctions,  // 'f'
	login};

enum functionMasks {
	F1_F4 = 0,
	F5_F8 = 4,
	F1 = 0b1,
	F2 = 0b10,
	F3 = 0b100,
	F4 = 0b1000,
	F5 = 0b10000,
	F6 = 0b100000,
	F7 = 0b1000000,
	F8 = 0b10000000};

enum handControllerCommands {
	ADDRES = '1',
	_SPEED = '2',			// 2
	HEADLIGHT = '3',				// 3
	FUNCTION_BANK = '4',			// 4
	FUNCTION = '5',				// 5
	DECODER_SELECTED = '6',  		// 6
	TRAIN_DELETED = '7',			// 7
	POWER = '8',
	sendHandShake = '9', } ;				// 8

/***** VARIABLES *****/
unsigned char mode = 0;
unsigned char caseSelector;	
unsigned char functionBank;	
byte handController = 0;

static void handControllerInstruction(unsigned char _command) {
	Serial.print("$$");
	Serial.write(_command); }


static unsigned char makeAddres(unsigned char b) {
	b -= '0'; 
	selectedAddress *= 10;
	selectedAddress += b;
	if(!makeNumberT) selectedAddress = b;			// if timeout occured, new number is formed
	if(selectedAddress > 127) selectedAddress = b; 	// handles address overflow 

		handControllerInstruction(ADDRES);
		Serial.write(selectedAddress);// }
	makeNumberT = 200; // after 2 second number starts over
	return 1; }



#define serialCommand(x) unsigned char x##F(unsigned char b)
#define printFunction(x) Serial.print(#x);Serial.print(' ');if(currentTrain.functions& x )Serial.println("ON");else Serial.println("OFF")
//#define function2handcontroller 


serialCommand( handControllerCommand ) { 
	Train currentTrain = getTrain( selectedAddress );																	// fetch variables of current train

	if(b>='A' && b <='D') {																								 // F1 - F4 / F5 - F8 depending on the state of 'functionBank'
		unsigned char functionShift = F1_F4, pin;
		unsigned char state = false;

		if( functionBank ) {
			functionShift = F5_F8;	
		}	

		functionShift = ( 1 << ( functionShift + ( b - 'A' ) ) );
		for( pin = 0; pin < 8; pin++ ) {
			if(functionShift & (1 << pin)) break; // stores which pin has changed in pin
		}

		
		currentTrain.functions ^= functionShift; // toggles one of the functions
		
		if( currentTrain.functions & functionShift) state = '1';
		else												state = '0';

		handControllerInstruction(FUNCTION); Serial.write(pin + '1');Serial.write(state); 
	}		

	else if(b == 'E'){
		currentTrain.speed |= ESTOP_MASK; 
		setSpeed( selectedAddress, currentTrain.speed ); }
	else {
		currentTrain.speed &= ~ESTOP_MASK;
		switch(b) {

			case '#': 
			functionBank ^= 0x1; 
			handControllerInstruction(FUNCTION_BANK);
			if(functionBank){ Serial.write('1'); } 
			else {			  Serial.write('0'); } 	// toggle this 1 bit so we can select F1-F4 and F5-F8
			break; 			

			case 'P': 
			handControllerInstruction(POWER);
			if(digitalRead(power_on)) {
				digitalWrite(power_on, LOW);
				Serial.write('0');
			}
			else {
				digitalWrite(power_on, HIGH);
				Serial.write('1');
			}
			break;	

			case 'S': currentTrain.speed &= 0b11000000;currentTrain.speed|=28; break;	 // S
			// case ':': currentTrain.speed-=3;packetType = speedPacket;	break;											 // <<	:
			// case ';': currentTrain.speed+=3;packetType = speedPacket;	break;											 // >>	;
			// case '<': currentTrain.speed--; packetType = speedPacket;	break;											 // <
			// case '>': currentTrain.speed++; packetType = speedPacket;	break;											 // >

			case '*': currentTrain.headLight ^= 0x1; 
			handControllerInstruction(HEADLIGHT);
			if(currentTrain.headLight) { Serial.write('1'); }
			else{ 				 		 Serial.write('0'); }
			break; 
		}
	}
	return 1; 
}


serialCommand(setDecoderType){	
	Train currentTrain = getTrain( selectedAddress );

	b -= '0';
	b = constrain(b,0,3);
	switch(b) {
		case MM2:	currentTrain.decoderType = MM2; break;
		case DCC14: currentTrain.decoderType = DCC14; break;
		case DCC28: currentTrain.decoderType = DCC28; break;
		default:	currentTrain.decoderType = EMPTY_SLOT; break; }

	EEPROM.write(selectedAddress, b);														// store in EEPROM
	if(b != EMPTY_SLOT) {
		handControllerInstruction(DECODER_SELECTED); Serial.write(b); }
	else {
		currentTrain.speed = 0;
		currentTrain.functions = 0;
		currentTrain.headLight = 0;
		if(!handController) Serial.print("Train ");Serial.print(selectedAddress);Serial.println(" deleted!");
		handControllerInstruction(TRAIN_DELETED); Serial.write(selectedAddress); } 
	return 1; }

serialCommand(newSpeed) {
	b = constrain(b, 0, 56);
	handControllerInstruction(_SPEED); Serial.write( b );
	
	setSpeed( selectedAddress, b );
	return 1;}

serialCommand(login) {
    handControllerInstruction(sendHandShake); // return handshake to handcontroller
	//Serial.write('0');

	connectT = 5;		// set timeout time at 10 seconds
	handController = 1; // handcontroller is found
	return 1; }
	
#undef serialCommand

unsigned char setAddres(unsigned char b) {				// OBSOLETE
	selectedAddress = constrain(b,1,80);
	if(!handController)Serial.print("selected addres = ");Serial.println(selectedAddress);
	return 1;}

unsigned char setFunctions(unsigned char b) {
	//currentTrain.functions = b;
	return 1; }


unsigned char setTrain(signed char b) {											// THIS FUNCTION HAS BECOME OBSOLETE FOR NOW, IT MAY BE USEFULL FOR THE FUTURE FOR USE WITH THE COMPUTER
	Train currentTrain = getTrain( selectedAddress );	

	switch(caseSelector) {
		case 0: selectedAddress = b;	break;
		case 1: currentTrain.speed = b; break;
		case 2: if(b&1) currentTrain.headLight |= 1;	break;	 
		case 3:
		currentTrain.functions = b; 
		Serial.print("selected addres: ");Serial.println(selectedAddress);
		Serial.print("speed of train is set at: ");Serial.println(currentTrain.speed);
		Serial.print("headlight is turned: ");if(currentTrain.headLight)Serial.println("ON");else Serial.println("OFF");
		Serial.print("funcion F1 - F8 ");Serial.println((unsigned char)currentTrain.functions, BIN);
		//newInstructionFlag = true;
		mode = IDLE;
		return 1;
		break;}
		
	caseSelector++; // if function is not ready, return 0 
	return 0;}

/***** SERIAL PROCESSING *****/
#define serialCommand(x) case x: if(x##F(b)) mode = IDLE; break;

void readSerialBus() {
	if(Serial.available()) {
		signed char b = Serial.read();
		//Serial.println((char)b);
		//PORTB ^= ( 1 << 5 );

		switch(mode) {										// the first unsigned char contains the instruction, the following unsigned chars are for that specific instrunction. When an instruction is processed it returns 1 
			case IDLE:											// and set mode back at IDLE for a new instruction. caseSelector is used for more than 1 follow-up unsigned chars. Could use a new name though
			caseSelector = 0;	
			//CLEAR_PHONE

			     if(b >= '0' && b <= '9') makeAddres(b);	 
			else if(b >= 'a' && b <= 'g') mode = b;	 
			break;

			serialCommand(handControllerCommand); // 'a'
			serialCommand(setDecoderType);		 // 'b'					
			//serialCommand(PROCESSING_COMMAND:	 // 'c'					 
			//serialCommand(TRACK_COMMAND:		 // 'd'									 
			serialCommand(newSpeed);			 // 'e'
			//serialCommand(SETTING_FUNCTIONS);  // 'f'
			serialCommand(login);				 // 'g'								 
																						 
		//if(stateChanged()) transmittChanges(); 
		} } }		// if a state of a decoder has changed, the central has to know NOT YET IMPLEMENTED
#undef serialCommand

void connect() {
	if(!connectT) {			// if connection timer expires..
		handController = 0; // .. no handcontrolelr
		connectT = 10;      // retry every 10 seconds
		digitalWrite(power_on, LOW);

		Serial.print("%%"); // Signal to central to reboot bluetooth connection
		Serial.write(0x82);
	}
}

