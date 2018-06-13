/******************************************************************************
 * client.c
 * Program 1 client
 * Aaron Berns
 * 2/5/18
 *
 * client.c is passed the address and port number of the chat server on the
 * command line, gets a username and an initial message and connects to the
 * specified chat server. Messages are sent back and forth until either the
 * client or server types "\quit" at which point client.c exits.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "gpio_driver.h"
#include <omp.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>

#define MAXDATASIZE 600


// connection functions from Beej's Guide 

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int estConnection(char *host, char *port) {
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through results and connect to first available
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return -2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);

    freeaddrinfo(servinfo);

    return sockfd;
}

void closeConnection(int sockfd) {
    close(sockfd);
}

void sendEvent(int sockfd, char *event) {
	// send message to server
    int len = strlen(event);
    int bytes_sent = send(sockfd, event, len, 0);
}

int getConfirm(int sockfd) {
	
	int numbytes;
	char buf[4];
	numbytes = recv(sockfd, buf, 3, 0);
	buf[3] = '\0';
	if (strcmp(buf, "rec") == 0) {
		return 1;
	}
	return 0;
}

void flashLed(int led, int times) {


	for (int i = 0; i < times; ++i) {
		digitalWrite(led, 0);
		sleep(1);
		digitalWrite(led, 1);
		sleep(1);
	}

	digitalWrite(led, 0);
}

int main(int argc, char *argv[])
{
	char *sound = "sound";
	char *disconnect = "disconnect";
	char *smoke = "smoke";
	char *motion = "motion";

	int end = 0; // shared varible to kill all threads before shutdown

	// connect to specified server and listening port
	int sockfd = estConnection(argv[1], argv[2]);

	// assign pins
	const int red_led = 21; // smoke
	const int blue_led = 5; // power
	const int green_led = 16; // motion
	const int yellow_led = 20; // sound
	const int sound_sensor = 12;
	const int touch_sensor = 19;
	const int motion_sensor = 13;
	const int smoke_sensor = 6;



	// use /dev/gpiomem, don't need root, safer
	pioInitGpio();
	
	// assign modes
	pinMode(red_led, 1);
	pinMode(blue_led, 1);
	pinMode(green_led, 1);
	pinMode(yellow_led, 1);
	pinMode(sound_sensor, 0);
	pinMode(touch_sensor, 0);
	pinMode(motion_sensor, 0);
	pinMode(smoke_sensor, 0);

	digitalWrite(blue_led, 1);


	omp_set_num_threads(6);
	#pragma omp parallel sections default(none), shared(end, sockfd, smoke, sound, motion)
	{
		#pragma omp section
		{
			// variables for temp sensor, help from bradsrpi.blogspot.com
			char buf[256];
			char tmpData[6];
			char path[] = "/sys/bus/w1/devices/28-051693b4daff/w1_slave";
			ssize_t numRead;
			float temp;
			char message[13];
			memset(&message, sizeof message, '\0');
			char oldMessage[13];
			memset(&oldMessage, sizeof oldMessage, '\0');

			int fd;
			while (!end) {
				fd = open(path, O_RDONLY);
				while ((numRead = read(fd, buf, 256)) > 0) {
					strncpy(tmpData, strstr(buf, "t=") +2, 5); 
					temp = strtof(tmpData, NULL);
					temp = (temp / 1000) * (9/5) + 32;
					sprintf(message, "temp %.1f F", temp);
				}

				if (strcmp(message, oldMessage) != 0) {
					sendEvent(sockfd, message);
					strcpy(oldMessage, message);
				}
				close(fd);
				sleep(2);
			}
		}
		#pragma omp section
		{
			while (!end) {
				if (digitalRead(sound_sensor)) {
					digitalWrite(yellow_led, 1);
					sendEvent(sockfd, sound);
					sleep(1);
					digitalWrite(yellow_led, 0);
		
				}
			}
		}
		#pragma omp section
		{
			while (!end) {
				
				if (digitalRead(motion_sensor)) {
					digitalWrite(green_led, 1);
					sendEvent(sockfd, motion);
					sleep(1);
					digitalWrite(green_led, 0);
		
				}
			}
		}

		#pragma omp section
		{
			while (!end) {
				
				if (digitalRead(smoke_sensor)) {
					digitalWrite(red_led, 1);
					sendEvent(sockfd, smoke);
					sleep(1);
					digitalWrite(red_led, 0);
				}
			}
		}
		#pragma omp section
		{ 
			while(!end) {
				if (digitalRead(touch_sensor)) {
					end = 1;
				}
			}
		}
	}
	flashLed(blue_led, 1);
	sendEvent(sockfd, disconnect);
	close(sockfd);
	
	return 0;
}
