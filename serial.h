extern unsigned char mode;

extern void readSerialBus();

#define CLEAR_PHONE Serial1.write(0x0C);
