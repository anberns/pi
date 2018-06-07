#include "gpio_driver.h"
#include <unistd.h>
#include <stdio.h>

int main() {

	// assign pins
	const int red_led = 21;
	const int sound_sensor = 19;

	// use /dve/gpiomem, don't need root, safer
	pioInitGpio();
	
	// assignn modes
	pinMode(red_led, 1);
	pinMode(sound_sensor, 0);

	// flash lights
	int i;
	while (1) {
		if (digitalRead(sound_sensor)) {
			digitalWrite(red_led, 1);
			sleep(1);
			digitalWrite(red_led, 0);
		}
	}
	
	
	return 0;
}
