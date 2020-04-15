#include <Arduino.h>

enum packetTypes {
	speedPacket = 1,
	functionPacket1,
	functionPacket2,
	functionPacket3 };

enum decoderTypes {
	MM2,
	DCC14,
	DCC28,
	SELECTRIX,
	EMPTY_SLOT = 255};

extern byte debug;
extern unsigned char packetType, newInstructionFlag;
extern int selectedAddres, currentAddres;

/***** CONSTANTS *****/
#define DCC_ZERO_BIT 1855
#define DCC_ONE_BIT 927

#define CLEAR_SPEED 0b11100000

#define ESTOP 30

extern void initDCC();
extern void DCCsignals();

enum trainCommands {
	ESTOP_MASK		= 0x40,
	REVERSE_MASK	= 0x80,
	FORWARD			= 0b011	<< 5,
	REVERSE			= 0b010	<< 5,
	FUNCTIONS1		= 0B100	<< 5, 
	FUNCTIONS2		= 0B1011 << 4,
	FUNCTIONS3		= 0B1010 << 4 };	 // F9-12 not yet in use, just added this for future purposes

