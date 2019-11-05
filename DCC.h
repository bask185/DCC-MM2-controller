#ifndef DCC_H
#define DCC_H

enum decoderTypes {
	MM2,
	DCC14,
	DCC28,
	EMPTY_SLOT = 255};

/***** CONSTANTS *****/
#define DCC_ZERO_BIT 1855
#define DCC_ONE_BIT 927
#define MAXIMUM_CURRENT 160
#define CLEAR_SPEED 0b11100000
#define directionPin2 12 // PE6
#define directionPin 13  // PB4
#define power_on 11 // port B
#define currentSensePin A4
#define ESTOP 30

extern void initDCC();
extern void DCCsignals();

enum trainCommands {
	ESTOP_MASK = 0x40,
	REVERSE_MASK = 0x80,
	FORWARD		= 0b011	<< 5,
	REVERSE		= 0b010	<< 5,
	FUNCTIONS1 = 0B100	<< 5, 
	FUNCTIONS2 = 0B1011 << 4,
	FUNCTIONS3 = 0B1010 << 4 };	 // F9-12 not yet in use, just added this for future purposes

extern unsigned char newInstructionFlag;

#endif