simple_led_loop: simple_led_loop.c
	gcc -o simple_led_loop simple_led_loop.c gpio_driver.c

motion: motion.c
	gcc -o motion motion.c gpio_driver.c

motion_touch: motion_touch.c
	gcc -o motion_touch motion_touch.c gpio_driver.c


touch: touch.c
	gcc -o touch touch.c gpio_driver.c

sound: sound.c
	gcc -o sound sound.c gpio_driver.c

clear: clear.c
	gcc -o clear clear.c gpio_driver.c

pi_server: pi_server.c
	gcc -o pi_server pi_server.c gpio_driver.c -lwiringPi

pi_client: pi_client.c
	gcc -o pi_client pi_client.c gpio_driver.c -lm -fopenmp 

pi_client_poll: pi_client_poll.c
	gcc -o pi_client_poll pi_client_poll.c gpio_driver.c -lm -fopenmp 
