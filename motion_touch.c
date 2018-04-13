#include "gpio_driver.h"
#include <unistd.h>
#include <stdio.h>

int main() {

	// assign pins
	const int red_led = 21;
	const int motion_det = 12;
	const int touch = 19;

	// use /dve/gpiomem, don't need root, safer
	pioInitGpio();
	
	// assignn modes
	pinMode(red_led, 1);
	pinMode(motion_det, 0);
	pinMode(touch, 0);

	// flash lights
	while (1) {
		if (digitalRead(motion_det)) {
			digitalWrite(red_led, 1);
			sleep(1);
			digitalWrite(red_led, 0);
		}
		if (digitalRead(touch)) {
			break;
		}
	}
	digitalWrite(red_led, 0);
	
	return 0;
}
