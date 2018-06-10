#include "gpio_driver.h"
#include <unistd.h>
#include <stdio.h>

int main() {

	// assign pins
	const int red_led = 21;
	const int touch_sensor = 19;

	// use /dve/gpiomem, don't need root, safer
	pioInitGpio();
	
	// assignn modes
	pinMode(red_led, 1);
	pinMode(touch_sensor, 0);

	// flash lights
	int i;
	while (1) {
		if (digitalRead(touch_sensor)) {
			digitalWrite(red_led, 1);
			sleep(1);
			digitalWrite(red_led, 0);
		}
	}
	
	
	return 0;
}
