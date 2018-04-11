all: simple_led_loop.c
	gcc -o simple_led_loop simple_led_loop.c gpio_driver.c
