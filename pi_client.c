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

#define MAXDATASIZE 600

/******************************************************************************
* function from Beej's Guide to fill in address structs
******************************************************************************/

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/******************************************************************************
* connection function, passed host name and port number, returns sockfd
* most of function from Beej's Guide
******************************************************************************/

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

/******************************************************************************
* close connection function, passed sockfd
******************************************************************************/

void closeConnection(int sockfd) {
    close(sockfd);
}

/******************************************************************************
* messaging function, passed sockfd for connection and user handle
* Gets message from user, sends to server, gets response and prints to screen.
* Repeats until user enters or server returns "\quit".
******************************************************************************/

void messaging(int sockfd, char *handle) {
    int numbytes, len, bytes_sent;
    char buf[MAXDATASIZE]; // stores server response
    char *input = NULL; // for getline(), user input
    char fullInput[650]; // for input and handle to sent to server
    size_t bufferSize = 0;
    int numChars = -5;

    // clear \n from buffer
    // help from stackoverflow questions 7898215
    char c;
    while ((c = getchar()) != '\n' && c != EOF) { }

    while(1) {
        
	// print user prompt
        printf("%s%s", handle, "> ");

        // get user input
        // help from cs344 lecture by Brewster
        numChars = getline(&input, &bufferSize, stdin);

        // check for "\quit" command from client
        if (strcmp(input, "\\quit\n") == 0) {
            break;
        }

        // add handle to outgoing message
        // help from stackoverflow question 308695
        snprintf(fullInput, sizeof fullInput, "%s%s%s", handle, "> ", input);

	// send message to server
        len = strlen(fullInput);
        bytes_sent = send(sockfd, fullInput, len, 0);
        free(input);
        input = NULL;

	// get server response
        numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0);
        buf[numbytes] = '\0';

        // check for "\quit" command from server
        if (strcmp(buf, "\\quit") == 0) {
            break;
        }

	// print server response
        printf("%s", buf);
        printf("%s", "\n");
    }
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

	digitalWrite(led, 0);

	for (int i = 0; i < times; ++i) {
		digitalWrite(led, 1);
		sleep(1);
		digitalWrite(led, 0);
	}
}

int main(int argc, char *argv[])
{
	char *sound = "sound";
	char *disconnect = "disconnect";

	// connect to specified server and listening port
	int sockfd = estConnection(argv[1], argv[2]);

	// assign pins
	const int red_led = 21;
	const int sound_sensor = 12;
	const int touch_sensor = 19;

	// use /dve/gpiomem, don't need root, safer
	pioInitGpio();
	
	// assign modes
	pinMode(red_led, 1);
	pinMode(sound_sensor, 0);
	pinMode(touch_sensor, 0);

	// flash lights
	int i;
	while (1) {
		if (digitalRead(sound_sensor)) {
			digitalWrite(red_led, 1);
			sendEvent(sockfd, sound);
			sleep(1);

			if (getConfirm(sockfd)) {
				digitalWrite(red_led, 0);
			}
			else {
				printf("failure: sound\n");
				flashLed(red_led, 3);
			}
			sleep(1);
		}
		if (digitalRead(touch_sensor)) {
			sendEvent(sockfd, disconnect);
			break;
		}
	}
	
	return 0;
}
