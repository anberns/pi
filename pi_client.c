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

int main(int argc, char *argv[])
{
	int numbytes;
	char buf[MAXDATASIZE];
	/* get handle from user, limit to 10 chars
	char handle[11];
	printf("Enter a username: ");
	scanf("%s", handle);
	*/

	// connect to specified server and listening port
	int sockfd = estConnection(argv[1], argv[2]);

	/* begin messaging if no connection errors
	if (sockfd >= 0) {
    		messaging(sockfd, handle);

		// close messaging socket after \quit from client or server
		closeConnection(sockfd);
	}

	// if connection error, return errno
	else {
		return sockfd;
	}
	*/

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
		perror("recv");
		exit(1);
	}
	buf[numbytes] = '\0';
	printf("%s\n", buf);
	close(sockfd);

	return 0;
}
