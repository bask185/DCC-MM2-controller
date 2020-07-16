#include "Serial.h"
#include "src/basics/timers.h"
#include "src/basics/io.h"
//#include "config.h"
#include <Arduino.h>
#include "DCC.h"
#include <EEPROM.h>

enum serialCommands { // 0x00 to 0x08 are the 8 primary commands. Each instruction will have followUp uint8_ts
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
	F8 = 0b10000000
};

enum handControllerCommands {
	ADDRES = '1',
	SPEED = '2',			// 2
	HEADLIGHT = '3',				// 3
	FUNCTION_BANK = '4',			// 4
	FUNCTION = '5',				// 5
	DECODER_SELECTED = '6',  		// 6
	TRAIN_DELETED = '7',			// 7
	POWER = '8',
	sendHandShake = '9', 
} ;				// 8

/***** VARIABLES *****/
uint8_t mode = 0;
uint8_t caseSelector;	
uint8_t functionBank;	
uint8_t settingDecoderType; // flag used to overrule handshaking when decoder type is being set.
uint8_t handController = 0;

static void handControllerInstruction(uint8_t _command) {
	connectT = 10; // after every instruction set the connection timeOut at 10 seconds
	Serial.print("$$");
	Serial.write(_command);
}


static uint8_t makeAddres(uint8_t b) {
	b -= '0'; 
	selectedAddress *= 10;
	selectedAddress += b;
	if( !makeNumberT ) selectedAddress = b;			// if timeout occured, new number is formed
	if(selectedAddress > nTrains) selectedAddress = b; 	// handles address overflow 

	Serial.write(selectedAddress);// 

	makeNumberT = 200; // after 2 second number starts over
	return 1; }



#define serialCommand(x) uint8_t x##F(uint8_t serialByte)
#define printFunction(x) Serial.print(#x);Serial.print(' ');if(currentTrain.functions& x )Serial.println("ON");else Serial.println("OFF")
//#define function2handcontroller 
	

serialCommand( handControllerCommand ) { 
	uint8_t functionShift, pin;
	uint8_t _state = false;
	Train currentTrain = getTrain( selectedAddress );	// fetch status of current train																// fetch variables of current train

	switch( serialByte ) {
	// TRAIN FUNCTIONS 
	case 'A':
	case 'B':
	case 'C':
	case 'D':

		if( functionBank )	{ functionShift = F5_F8; }	
		else 				{ functionShift = F1_F4; }

		pin = 1 << ( serialByte - 'A' );  
		pin <<= functionShift;	// either f1-f4 or f5-f8

		if( currentTrain.functions & pin ) { 		// if function was already on, we toggle it to 0
			_state = '0';
			Serial.println( ~pin );
			setFunctions( selectedAddress, ~pin ); 	// send the inverse
		}
		else {										// if function was already on, we toggle it to 1
			_state = '1';
			Serial.println( pin );
			setFunctions( selectedAddress,  pin );	// send bit
		}

		handControllerInstruction(FUNCTION); Serial.write(pin + '1');Serial.write(_state); 
		break;

	// EMERGENCY STOP
	case 'E':
		currentTrain.speed |= ESTOP_MASK; 
		goto updateSpeed;

		//currentTrain.speed &= ~ESTOP_MASK;

	case '#': 
		functionBank ^= 0x1; 
		handControllerInstruction(FUNCTION_BANK);
		if(functionBank){ Serial.write('1'); } 
		else {			  Serial.write('0'); } 	// toggle this 1 bit so we can select F1-F4 and F5-F8
		break; 			

	case 'P': 
		handControllerInstruction(POWER);
		if(digitalRead(power_on)) {
			turnPower( _OFF );
			Serial.write('0');
		}
		else {
			turnPower( _ON );
			Serial.write('1');
		}
		break;	

	case 'S':
		currentTrain.speed &= 0b11000000;
		currentTrain.speed |= 28; 
		goto updateSpeed;
		

	case ':': 
		currentTrain.speed -= 3; 										 // <<	:
		currentTrain.speed = constrain( currentTrain.speed, 0, 56 );
		goto updateSpeed;

	case ';': 
		currentTrain.speed += 3;												 // >>	;
		currentTrain.speed = constrain( currentTrain.speed, 0, 56 );
		goto updateSpeed;

	case '<': 
		currentTrain.speed --  ; 												 // <
		currentTrain.speed = constrain( currentTrain.speed, 0, 56 );
		goto updateSpeed;

	case '>': 
		currentTrain.speed ++  ; 												 // >
		currentTrain.speed = constrain( currentTrain.speed, 0, 56 );
		goto updateSpeed;
		
		break;

	case '*': currentTrain.headLight ^= 0x1; // fetch current state and toggle it
		setHeadlight( selectedAddress, currentTrain.headLight );

		handControllerInstruction(HEADLIGHT);
		if(currentTrain.headLight)	Serial.write('1');
		else						Serial.write('0');
		break; 

	// case 'addNewInstructions'
	
	updateSpeed:
		//currentTrain.speed = constrain( currentTrain.speed, 0, 56 ); this constrain cause trouble with the stop and direction flags
		setSpeed( selectedAddress, currentTrain.speed );
		handControllerInstruction(SPEED); Serial.write( currentTrain.speed );


	} // end of switch case

	return 1; 
}

// NOT YET WORKING, THE HANDSHAKING SIGNAL MAY INTERFERE
serialCommand(setDecoderType) {	
	Train currentTrain = getTrain( selectedAddress );

	serialByte -= '0';
	serialByte = constrain(serialByte, 0, 3);

	switch( serialByte ) {
	case MM2:	currentTrain.decoderType = MM2;			break;
	case DCC14: currentTrain.decoderType = DCC14; 		break;
	case DCC28: currentTrain.decoderType = DCC28;		break;
	default:	currentTrain.decoderType = EMPTY_SLOT; 	break;
	}

	EEPROM.write(selectedAddress, serialByte);														// store in EEPROM
	if(serialByte != EMPTY_SLOT) {
		handControllerInstruction(DECODER_SELECTED); Serial.write( serialByte );
	}
	else {
		handControllerInstruction(TRAIN_DELETED); Serial.write(selectedAddress);
	} 
	return 1; }

serialCommand(newSpeed) {
	uint8_t _speed = serialByte;
	_speed = constrain(_speed, 0, 56);
	handControllerInstruction(SPEED); Serial.write( _speed );
	
	setSpeed( selectedAddress, serialByte );
	return 1;}

serialCommand(login) {
    handControllerInstruction( sendHandShake ); // return handshake to handcontroller
	//Serial.write('0');

	connectT = 10;		// set timeout time at 10 seconds
	handController = 1; // handcontroller is found
	return 1; }
	
#undef serialCommand

// uint8_t setAddres(uint8_t serialByte) {				// OBSOLETE
// 	selectedAddress = constrain(serialByte, 1, nTrains - 1 );
// 	if(!handController)Serial.print("selected addres = ");Serial.println(selectedAddress);
// 	return 1;}

// uint8_t setFunctions(uint8_t serialByte) {
// 	//currentTrain.functions = b;
// 	return 1; }


// uint8_t setTrain(signed char serialByte) {											// THIS FUNCTION HAS BECOME OBSOLETE FOR NOW, IT MAY BE USEFULL FOR THE FUTURE FOR USE WITH THE COMPUTER
// 	Train currentTrain = getTrain( selectedAddress );	

// 	switch(caseSelector) {
// 		case 0: selectedAddress = serialByte;						break;
// 		case 1: currentTrain.speed = serialByte; 					break;
// 		case 2: if(serialByte & 1 ) currentTrain.headLight |= 1;	break;	 
// 		case 3:
// 		currentTrain.functions = serialByte; 
// 		Serial.print("selected addres: ");			Serial.println(selectedAddress);
// 		Serial.print("speed of train is set at: ");	Serial.println(currentTrain.speed);
// 		Serial.print("headlight is turned: ");		if(currentTrain.headLight)Serial.println("ON");else Serial.println("OFF");
// 		Serial.print("funcion F1 - F8 ");			Serial.println((uint8_t)currentTrain.functions, BIN);
// 		//newInstructionFlag = true;
// 		mode = IDLE;
// 		return 1;
// 		break;}
		
// 	caseSelector++; // if function is not ready, return 0 
// 	return 0; }

/***** SERIAL PROCESSING *****/
#define serialCommand(x) case x: if(x##F(serialByte)) mode = IDLE; break;

void readSerialBus() {
	if(Serial.available()) {
		signed char serialByte = Serial.read();
		//Serial.println((char)b);
		//PORTB ^= ( 1 << 5 );

		switch(mode) {										// the first uint8_t contains the instruction, the following uint8_ts are for that specific instrunction. When an instruction is processed it returns 1 
			case IDLE:											// and set mode back at IDLE for a new instruction. caseSelector is used for more than 1 follow-up uint8_ts. Could use a new name though
			caseSelector = 0;	
			//CLEAR_PHONE

			     if(serialByte >= '0' && serialByte <= '9') makeAddres( serialByte );	 
			else if(serialByte >= 'a' && serialByte <= 'g') mode = serialByte;	 
			break;

			serialCommand(handControllerCommand);// 'a'
			serialCommand(setDecoderType);		 // 'b'					
			//serialCommand(PROCESSING_COMMAND:	 // 'c'					 
			//serialCommand(TRACK_COMMAND:		 // 'd'		
			case 'd': dumpData(); break;							 
			serialCommand(newSpeed);			 // 'e'
			//serialCommand(SETTING_FUNCTIONS);  // 'f'
			serialCommand(login);				 // 'g'								 
																						 
		//if(stateChanged()) transmittChanges(); 
		} } }		// if a state of a decoder has changed, the central has to know NOT YET IMPLEMENTED
#undef serialCommand

void connect() {
	if( ( !connectT )/* && ( !settingAddress )*/ ) {			// if connection timer expires..
		connectT = 10;      // retry every 10 seconds
		handController = 0; // .. no handcontrolelr
		
		turnPower( _OFF );
	}
}

