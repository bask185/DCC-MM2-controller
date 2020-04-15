#include <Arduino.h>
extern unsigned char mode;

extern void readSerialBus();
extern void connect();


#define CLEAR_PHONE Serial.write(0x0C);
