#include <Arduino.h>


typedef struct trains_ {
	signed char speed;
	unsigned char headLight : 1;			
	unsigned char functions;
	unsigned char decoderType;
} Train;
//extern Trains train[];  TRAINS ARE NOW LOCAL TO DCC.c

extern Train getTrain( uint8_t );


enum decoderTypes {
	MM2,
	DCC14,
	DCC28,
	SELECTRIX,
	EMPTY_SLOT = 255};

extern byte debug;
extern uint8_t selectedAddress;

extern void setSpeed( uint8_t , uint8_t );
extern void setHeadlight( uint8_t , uint8_t );
extern void setFunctions( uint8_t , uint8_t );

// CONTROL FUNCTIONS

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

