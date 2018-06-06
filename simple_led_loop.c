#include "gpio_driver.h"
#include <unistd.h>
#include <stdio.h>

int main() {

	// assign pins
	const int red_led = 21;
	const int blue_led = 5;
	const int green_led = 16;
	const int yellow_led = 20;

	// use /dve/gpiomem, don't need root, safer
	pioInitGpio();
	
	// assignn modes
	pinMode(red_led, 1);
	pinMode(blue_led, 1);
	pinMode(green_led, 1);
	pinMode(yellow_led, 1);

	// flash lights
	int i;
	for (i = 0; i < 5; ++i) {
		digitalWrite(red_led, 1);
		digitalWrite(blue_led, 1);
		digitalWrite(green_led, 1);
		digitalWrite(yellow_led, 1);
		sleep(1);
		digitalWrite(red_led, 0);
		digitalWrite(blue_led, 0);
		digitalWrite(green_led, 0);
		digitalWrite(yellow_led, 0);
		sleep(1);
	}
	
	
	return 0;
}
