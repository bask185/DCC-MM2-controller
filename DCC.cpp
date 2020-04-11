#include "DCC.h"
#include "config.h"
#include <Arduino.h>

enum packetTypes {
	speedPacket = 1,
	functionPacket1,
	functionPacket2,
	functionPacket3 };

enum states {
	assemblePacket = 1,
	awaitPacketSent,
	nextAddres,
	newPacketSent,
	nextPacketType };



unsigned char newInstructionFlag, packetSentFlag, lastAddresFlag, newPacketSentFlag, state = assemblePacket;

unsigned char transmittBuffer[8]; 
unsigned char *ptr;																																												// make pointer to point at the transmittBuffer																																							// F1-F4 / F5-F8																																 // for processing of serial data
unsigned char packetType = speedPacket, ISR_state = 0, sendPacket_state = 0, Bit;	// for DCC packets


/******** STATE FUNCTIONS ********/
#define stateFunction(x) static unsigned char x##F()
stateFunction(assemblePacket) {
	struct Packets {
		unsigned char addres;
		unsigned char speed;
		unsigned char functions1;
		unsigned char functions2;
		unsigned char checksum; } Packet;
		
	if(currentAddres) {
	/******	ADDRES	*******/ 
		Packet.addres = currentAddres;
		
		switch(packetType){																		// there are 3 types of packets, speed, F1-F4 + HL and F5-F8
			signed char speed_tmp;																// make local variable for speed to calculate with
		
		/******	SPEED	*******/ 
			case speedPacket:
			speed_tmp = train[currentAddres].speed;
			if(speed_tmp & REVERSE_MASK) {														// reverse operation
				speed_tmp &= ~REVERSE_MASK;
				Packet.speed = REVERSE; }
			else {																				// forward operation
				Packet.speed = FORWARD;	}														// store speed in 'speed_tmp' to calculate with

			if(speed_tmp & ESTOP_MASK) {
				//train[currentAddres].speed = 28;												// this causes problems? I think it does because it corrupts all following commands
				Packet.speed &= CLEAR_SPEED;													// sets the speed at E-stop without modifying the direcion bits
				Packet.speed |= 1; } 
			else {
				speed_tmp -= 28;
				if(speed_tmp&0x80) {
					speed_tmp ^= 0xff;
					speed_tmp++; }
				if(train[currentAddres].decoderType == DCC14) speed_tmp /= 2;
				if(speed_tmp == 0) {															
					Packet.speed &= CLEAR_SPEED; }// stop										// sets the speed at 0 without modifying the direction bits
				else {						
					if(train[currentAddres].decoderType==DCC28) {								// equal speed steps for DCC28 need the 5th bit set
						Packet.speed |= (speed_tmp+3)/2;
						if(speed_tmp%2==0) Packet.speed |= (1<<4); }							// set extra speed bit for DCC 28 decoders
					if(train[currentAddres].decoderType==DCC14) {
						Packet.speed |= (speed_tmp+1);
						if(train[currentAddres].headLight) Packet.speed |= (1<<4); } } }		// set headlight bit for DCC 14 decoders NOTE might be unneeded as we might be able to this in the first function unsigned char
			
			transmittBuffer[3] = Packet.speed; 
			break;
		
			/******	FUNCTIONS F1 - F4	*******/	
			case functionPacket1:
			Packet.functions1 = FUNCTIONS1;																						 // mark this as instructions unsigned char
			Packet.functions1 |= (train[currentAddres].functions & 0x0F);						// F1 - F4
			if(train[currentAddres].headLight && train[currentAddres].decoderType == DCC28) {
				Packet.functions1 |= (1<<4); }					 								// turns light on for DCC 28 decoders, the if statement might be useless as it may do no harm to do this for dcc 14 decoders as well
			transmittBuffer[3] = Packet.functions1;
			break; 
				
			/******	FUNCTIONS F5 - F8	*******/ 
			case functionPacket2: 
			Packet.functions2 = FUNCTIONS2;																						 // mark this as instructions unsigned char
			Packet.functions2 |= (train[currentAddres].functions >> 4);							// F5 - F8
			transmittBuffer[3] = Packet.functions2;
			break; }
			
			/******	FUNCTIONS F9 - F12	*******/
			/*case functionPacket3:																										 // NOTE future usage?
			Packet.functions3 = FUNCTIONS2;																						 // mark this as instructions unsigned char
			Packet.functions3 |= (train[currentAddres].functions2 >> 4);						// F9 - F12
			transmittBuffer[3] = Packet.functions3;
			break; }*/

		/***** CHECKSUM *******/	
		Packet.checksum = Packet.addres ^ transmittBuffer[3];									
		
		/***** SHIFT PACKETS FOR TRANSMITT BUFFER *******/ 
		transmittBuffer[0] = 0xFF;																// pre amble
		transmittBuffer[1] = 0xFC;	// pre-amble + 2 0 bits						 				// pre amble, 0 bit, 1st 0 bit of addres unsigned char
		transmittBuffer[2] = Packet.addres << 1;												// addres unsigned char, 0 bit
		//transmittBuffer[3] is filled above
		transmittBuffer[4] = Packet.checksum >> 1;												// 0 bit, 7 bits of checksum
		transmittBuffer[5] = Packet.checksum << 7 | 1<<6; }
	
//		11111111111111 0 11111111 0 00000000 0 11111111 1	<- IDLE PACKET
//
//		 0			 1			2			3			 4			5	
//	11111111	111111_0_1	 1111111_0	 00000000	 0_1111111 	11000000
//		0xFF	 0xFD		 0xFE		0x00			0x7F	0xC0
	
	else {																													// if current addres = 0,	
		transmittBuffer[0] = 0xFF;																		// we overwrite the buffers to 
		transmittBuffer[1] = 0xFD;																		// assemble an IDLE packet
		transmittBuffer[2] = 0xFE;																		
		transmittBuffer[3] = 0x00;																		
		transmittBuffer[4] = 0x7F;
		transmittBuffer[5] = 0xC0; }
	
	ptr = &transmittBuffer[0];																			// point ptr at first unsigned char
	packetSentFlag = false;																				 // clear flag
	bitSet(TIMSK1,OCIE1B);																					// send the unsigned char, enable interrupt
	return true; }
	
stateFunction(awaitPacketSent) {
	if(packetSentFlag)	return 1;
	else 				return 0;}
	
stateFunction(nextAddres) {
	unsigned char i;
	for(i=currentAddres+1; i<=80; i++){
		if(train[i].decoderType <= 2){;currentAddres=i; return 1;}}
	currentAddres = 0;																			// if no valid addres is found, adres 0 will be selected
	lastAddresFlag = 1;
	return 1; }
	
stateFunction(newPacketSent) {
	if(newInstructionFlag<60){ 																				// flag is secretely also used to count
		/*if(newInstructionFlag % 3 == 0) currentAddres = selectedAddres; *///every 3th cycle a new packet for the new instruction will be send
		//else	currentAddres = 0;																				// the other cycles will be send with idle packets
		newInstructionFlag++;}
		
	else newInstructionFlag = false;
	return 1; }
	
stateFunction(nextPacketType) {
	switch(packetType) {
		case speedPacket: 		packetType = functionPacket1; break;
		case functionPacket1:	packetType = functionPacket2; break;
		case functionPacket2:	packetType = speedPacket; 		break;}
	return 1; }

// STATE MACHINE
#define State(x) break; case x: /*Serial.println(#x);*/ if(x ## F())
extern void DCCsignals(void) {
	switch(state){
		default: state = assemblePacket;
		
		State(assemblePacket)		state = awaitPacketSent; 
		
		State(awaitPacketSent){	
			if(newInstructionFlag)	state = newPacketSent;
			else					state = nextAddres; }
		
		State(nextAddres){
			if(lastAddresFlag)		state = nextPacketType;
			else					state = assemblePacket; }
		
		State(newPacketSent){
			if(newPacketSentFlag)	state = assemblePacket;
			else					state = nextPacketType; }
		
		State(nextPacketType)		state = assemblePacket; 
		
		break; } }


/***** INTERRUPT SERVICE ROUTINE *****/		
ISR(TIMER1_COMPB_vect) {
	static unsigned char bitMask = 0x80;
	// if DCC
	
	PORTD ^= 0b00011000; // pin 3 and 4 needs to be toggled
	
	if(!ISR_state){															// pick a bit and set duration time accordingly		
		ISR_state = 1;

		if(*ptr & bitMask)	{OCR1A=DCC_ONE_BIT;	/* Serial.print('1');*/ }	// '1' 58us
		else				{OCR1A=DCC_ZERO_BIT; /* Serial.print('0');*/ }	// '0' 116us
		
		if(bitMask == 0x20 && ptr == &transmittBuffer[5]) {					// last bit?
			ptr = &transmittBuffer[0];
			bitClear(TIMSK1,OCIE1B);
			packetSentFlag = true;} 

		else {																// if not last bit, shift bit mask, and increment pointer if needed
			bitMask >>= 1;
			if(!bitMask) {
				bitMask = 0x80; 
				ptr++;} } }

	else {
		ISR_state = 0; } } // toggle pin 8 and 9 for direction*/

void initDCC() {					 // initialize the timer 
	cli();
	bitSet(TCCR1A,WGM10);		 
	bitSet(TCCR1A,WGM11);
	bitSet(TCCR1B,WGM12);
	bitSet(TCCR1B,WGM13);

	bitSet(TCCR1A,COM1B1);		
	bitSet(TCCR1A,COM1B0);

	bitClear(TCCR1B,CS12);		// set prescale on 1
	bitClear(TCCR1B,CS11);
	bitSet(TCCR1B,CS10);

	OCR1A=DCC_ONE_BIT;
	cli();
}// on/off time?
