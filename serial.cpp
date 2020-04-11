#include "Serial.h"
#include "dcc.h"
#include "src/basics/timers.h"
#include "src/basics/io.h"
#include "config.h"
#include <Arduino.h>
#include "DCC.h"
#include <EEPROM.h>

enum serialCommands { // 0x00 to 0x08 are the 8 primary commands. Each instruction will have followUp unsigned chars
	IDLE = 0,
	handControllerCommand = 'a',
	setDecoderType,
	processingCommand,
	trackCommand,
	setSpeed,
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
	HEADLIGHT,				// 3
	FUNCTION_BANK,			// 4
	FUNCTION,				// 5
	DECODER_SELECTED,  		// 6
	TRAIN_DELETED,			// 7
	POWER } ;				// 8

/***** VARIABLES *****/
unsigned char mode = 0;
int selectedAddres=0;	// must be int to prevent overflows during makeAddres()
unsigned char caseSelector;	
unsigned char functionBank;	
byte handController = 1;

static void handControllerInstruction(unsigned char _command) {
	Serial.print("$$");
	Serial.write(_command); }

static unsigned char makeAddres(unsigned char b) {
	b -= '0'; 
	selectedAddres *= 10;
	selectedAddres += b;
	if(!makeNumberT) selectedAddres = b;			// if timeout occured, new number is formed
	if(selectedAddres > 127) selectedAddres = b; 	// handles address overflow 
	if(!handController) {
		Serial.print("Selected addres = ");Serial.println(selectedAddres);
		Serial.print("decoder type = "); 
		switch(train[selectedAddres].decoderType) {
			case MM2: Serial.println("MM2");break;
			case DCC14: Serial.println("DCC14");break;
			case DCC28: Serial.println("DCC28");break;
			default: Serial.println(train[selectedAddres].decoderType);break;} }
	else {
		handControllerInstruction(ADDRES);
		Serial.write(selectedAddres); }
	makeNumberT = 200; // after 2 second number starts over
	return 1; }



#define serialCommand(x) unsigned char x##F(unsigned char b)
#define printFunction(x) Serial.print(#x);Serial.print(' ');if(train[selectedAddres].functions& x )Serial.println("ON");else Serial.println("OFF")
//#define function2handcontroller 


serialCommand(handControllerCommand) { 
	if(b>='A' && b <='D') {																								 // F1 - F4 / F5 - F8 depending on the state of 'functionBank'
		unsigned char functionShift = F1_F4, pin;
		unsigned char state = false;

		if(functionBank) functionShift = F5_F8;				 
		functionShift = (1 << (functionShift + b - 'A'));
		for(pin=0;pin<8;pin++) if(functionShift & (1 << pin)) break; // stores which pin has changed in pin
		train[selectedAddres].functions ^= functionShift; // toggles one of the functions
		if(train[selectedAddres].functions & functionShift) state = '1';
		else												state = '0';
		if(handController) {
			handControllerInstruction(FUNCTION); Serial.write(pin + '1');Serial.write(state); }
		else switch(functionShift) {
			case F1: printFunction(F1); break;
			case F2: printFunction(F2); break;
			case F3: printFunction(F3); break;
			case F4: printFunction(F4); break;
			case F5: printFunction(F5); break;
			case F6: printFunction(F6); break;
			case F7: printFunction(F7); break;
			case F8: printFunction(F8); break; } } 
	else if(b == 'E'){
		train[selectedAddres].speed |= ESTOP_MASK; }
	else {
		train[selectedAddres].speed &= ~ESTOP_MASK;
		switch(b) {
			case '#': functionBank ^= 0x1; 
			if(functionBank){ 
				if(!handController) {
					Serial.println("F5-F8"); }
				else {
					handControllerInstruction(FUNCTION_BANK); Serial.write('1'); } }
			else {
				if(!handController) {
					Serial.println("F1-F4"); }
				else {
					handControllerInstruction(FUNCTION_BANK); Serial.write('0'); } }	// toggle this 1 bit so we can select F1-F4 and F5-F8
			break; 											 
			case 'P': 
			digitalWrite(power_on, !digitalRead(power_on));	
			if(!handController) {													// P
				if(digitalRead(power_on))	Serial.println("power ON");
				else						Serial.println("power OFF"); }
			else {
				handControllerInstruction(POWER);
				if(digitalRead(power_on)) {Serial.write('1'); Serial.println("power ON");}
				else					  {Serial.write('0'); Serial.println("power OFF");} }
			break;		 
			case 'S': train[selectedAddres].speed &= 0b11000000;train[selectedAddres].speed|=28; break;											 // S
			case ':': train[selectedAddres].speed-=3;	break;											 // <<	:
			case ';': train[selectedAddres].speed+=3;	break;											 // >>	;
			case '<': train[selectedAddres].speed--;	break;											 // <
			case '>': train[selectedAddres].speed++;	break;											 // >
			case '*': train[selectedAddres].headLight ^= 0x1;
			if(!handController)	Serial.print("Headlight ");
			else {
				handControllerInstruction(HEADLIGHT); }
			if(train[selectedAddres].headLight) {
				if(!handController) Serial.println("ON "); else Serial.write('1'); }
			else{
				if(!handController) Serial.println("OFF"); else Serial.write('0'); }
			break; }	
		//train[selectedAddres].speed = constrain((train[selectedAddres].speed,0,56);
		if((train[selectedAddres].speed & 0b00111111) < 28) {train[selectedAddres].speed |=  REVERSE_MASK;Serial.println("REVERSE");}
		if((train[selectedAddres].speed & 0b00111111) > 28) {train[selectedAddres].speed &= ~REVERSE_MASK;Serial.println("FORWARD");}
		if(!handController){
			Serial.print("Speed = ");Serial.println((train[selectedAddres].speed)&0b00111111); }
		else {
			Serial.print("$$");Serial.write(_SPEED);Serial.write((train[selectedAddres].speed)&0b00111111); } }
	newInstructionFlag = 1;
	//if(train[selectedAddres].speed & ESTOP_MASK) {Serial.println("E-stop"); }
	return 1; }

serialCommand(setDecoderType){	
	b -= '0';
	b = constrain(b,0,3);
	switch(b) {
		case MM2:	train[selectedAddres].decoderType = MM2; break;
		case DCC14: train[selectedAddres].decoderType = DCC14; break;
		case DCC28: train[selectedAddres].decoderType = DCC28; break;
		default:	train[selectedAddres].decoderType = EMPTY_SLOT; break; }
		
	//Serial.println("setting decoder"); Serial.println(b);
	//Serial.print("selected address = ");Serial.println(selectedAddres);
	EEPROM.write(selectedAddres, b);														// store in EEPROM
	if(b != EMPTY_SLOT) {
		if(!handController) Serial.print("decoder selected: ");
		switch(b) {

			case MM2:	
			if(!handController) Serial.print("MM2 2");
			else handControllerInstruction(DECODER_SELECTED); Serial.write(b);
			break;

			case DCC14: 
			if(!handController) Serial.print("DCC 14 speed steps");
			else handControllerInstruction(DECODER_SELECTED); Serial.write(b);
			break;

			case DCC28: 
			if(!handController) Serial.print("DCC 28 speed steps");
			else handControllerInstruction(DECODER_SELECTED); Serial.write(b);
			break; } }
	else {
		train[selectedAddres].speed = 0;
		train[selectedAddres].functions = 0;
		train[selectedAddres].headLight = 0;
		if(!handController) Serial.print("Train ");Serial.print(selectedAddres);Serial.println(" deleted!");
		handControllerInstruction(TRAIN_DELETED); Serial.write(selectedAddres); } 
	return 1; }

serialCommand(setSpeed) {
	b = constrain(b,0,56);
	train[selectedAddres].speed = b;
	if(!handController) Serial.print("setting speed @ "); Serial.println(b);
	train[selectedAddres].speed = constrain(train[selectedAddres].speed,0,56);// should be redundant
	handControllerInstruction(_SPEED); Serial.write(b);
	return 1;}

serialCommand(login) {
    Serial.println("HAND CONTROLLER CONNECTED!");
	handController = 1;
	return 1; }
	
#undef serialCommand

unsigned char setAddres(unsigned char b) {				// OBSOLETE
	selectedAddres = constrain(b,1,80);
	if(!handController)Serial.print("selected addres = ");Serial.println(selectedAddres);
	return 1;}

unsigned char setFunctions(unsigned char b) {
	train[selectedAddres].functions = b;
	
	return 1; }


unsigned char setTrain(signed char b) {											// THIS FUNCTION HAS BECOME OBSOLETE FOR NOW, IT MAY BE USEFULL FOR THE FUTURE FOR USE WITH THE COMPUTER
	switch(caseSelector) {
		case 0: selectedAddres = b;	break;
		case 1: train[selectedAddres].speed = b; break;
		case 2: if(b&1) train[selectedAddres].headLight |= 1;	break;	 
		case 3:
		train[selectedAddres].functions = b; 
		Serial.print("selected addres: ");Serial.println(selectedAddres);
		Serial.print("speed of train is set at: ");Serial.println(train[selectedAddres].speed);
		Serial.print("headlight is turned: ");if(train[selectedAddres].headLight)Serial.println("ON");else Serial.println("OFF");
		Serial.print("funcion F1 - F8 ");Serial.println((unsigned char)train[selectedAddres].functions, BIN);
		newInstructionFlag = true;
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
		Serial.println((char)b);
		PORTB ^= ( 1 << 5 );

		switch(mode) {										// the first unsigned char contains the instruction, the following unsigned chars are for that specific instrunction. When an instruction is processed it returns 1 
			case IDLE:											// and set mode back at IDLE for a new instruction. caseSelector is used for more than 1 follow-up unsigned chars. Could use a new name though
			caseSelector = 0;	
			//CLEAR_PHONE
				 if(b >= '0' && b <= '9') makeAddres(b);	 
			else if(b >= 'a' && b <= 'f') mode = b;	 
			else {
				Serial.print("INVALID COMMAND ");Serial.println(b); }
			break;

			serialCommand(handControllerCommand); // 'a'
			serialCommand(setDecoderType);		 // 'b'					
			//serialCommand(PROCESSING_COMMAND:	 // 'c'					 
			//serialCommand(TRACK_COMMAND:		 // 'd'									 
			serialCommand(setSpeed);			 // 'e'
			//serialCommand(SETTING_FUNCTIONS);  // 'f'
			serialCommand(login);				 // 'g'								 
																						 
		//if(stateChanged()) transmittChanges(); 
		} } }		// if a state of a decoder has changed, the central has to know NOT YET IMPLEMENTED
#undef serialCommand
