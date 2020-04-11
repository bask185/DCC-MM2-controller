extern unsigned char mode;

extern void readSerialBus();

#define CLEAR_PHONE Serial.write(0x0C);
