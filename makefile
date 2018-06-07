all: pi_client pi_client_poll

pi_client: pi_client.c
	gcc -o pi_client pi_client.c gpio_driver.c -lm -fopenmp 

pi_client_poll: pi_client_poll.c
	gcc -o pi_client_poll pi_client_poll.c gpio_driver.c -lm -fopenmp 
