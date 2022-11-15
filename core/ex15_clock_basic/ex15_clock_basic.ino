#include <M5Stack.h>

#define TFT_GREY 0x5AEB

void setup(void) {
	M5.begin();
	M5.Power.begin();
	clock_init();
}

void loop() {
	clock_Needle();
	delay(100);
}
