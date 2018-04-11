#include "gpio_driver.h"
#include <unistd.h>
#include <stdio.h>

int main() {

	// assign pins
	const int red_led = 21;
	const int blue_led = 26;

	// use /dve/gpiomem, don't need root, safer
	pioInitGpio();
	
	// assignn modes
	pinMode(red_led, 1);
	pinMode(blue_led, 1);

	// flash lights
	int i;
	for (i = 0; i < 5; ++i) {
		digitalWrite(red_led, 1);
		digitalWrite(blue_led, 0);
		sleep(1);
		digitalWrite(red_led, 0);
		digitalWrite(blue_led, 1);
		sleep(1);
	}
	
	// clear all
	digitalWrite(red_led, 0);
	digitalWrite(blue_led, 0);
	
	return 0;
}
