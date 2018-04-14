/******************************************************************************
 * ftserver.c
 * Program 2 server 
 * Aaron Berns
 * 2/25/18
 *
 * ftserver accepts a port number between 1024 and 65535, opens a listening 
 * process and if contacted by a client, creates a child process that sends
 * the contents of the current working directory or transfers a given file, if 
 * it exists. The child process creates two ftp TCP connections, one control
 * the other data and closes both before terminating.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 20
#define BACKLOG 10

// handler to deal with zombies from Beej's guide
void sigchld_handler(int s) {
    //restore waitpid()
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

/******************************************************************************
* function from Beej's Guide to send all data 
******************************************************************************/
int sendall(int s, char *buf, int *len) {
    int total = 0;
    int bytesleft = *len;
    int n;

    while (total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -=n;
    }

    *len = total;

    return n == -1?-1:0;
}

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
* validPort() accepts a string port number and returns 0 unless the port
* number is reserved or out of range.
******************************************************************************/
int validPort(char *port) 
{
    int intPort = atoi(port);
    if (intPort < 1024 || intPort > 65535) {
        printf("Invalid port number\n");
        return 1;
    }
    return 0;
}

/******************************************************************************
* validCommand() accepts a string command and returns 0 unless the command 
* is invalid.
******************************************************************************/
int validCommand(char *buf)
{
    if(strcmp(buf, "-l") != 0 && (buf[0] != '-' && buf[1] != 'g')) {
        return 1;
    }
    return 0;
}

/******************************************************************************
* main
* Most of function from Beej's guide
******************************************************************************/
int main(int argc, char *argv[])
{
	
    ssize_t numbytes;
    int len, bytes_recv;
    char buf[MAXDATASIZE];
    char fName[MAXDATASIZE];
    char dPort[6];

    // to store file descriptors
    int listen_fd, new_fd;

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    int rv;
    char s[INET6_ADDRSTRLEN];


    // validate port from command line
    
    if (validPort(argv[1])) {
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // create listen port
    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through results and connect to first available
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((listen_fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                    sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
            
        }

        if (bind(listen_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(listen_fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(listen_fd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1 ) {
        perror("sigaction");
    }

    printf("server open on %s\n", argv[1]);

    // listen for incoming requests from client
    while (1) {
        sin_size = sizeof their_addr;

        // request from client, accept to create control TCP connection
        new_fd = accept(listen_fd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
                s, sizeof s);
        printf("Connection from %s\n", s);
			
		if (!fork()) {
			close(listen_fd);
			while (1) {
				memset(&buf, '\0', sizeof buf);
				numbytes = recv(new_fd, buf, MAXDATASIZE-1, 0);
				
				if (strcmp(buf, "sound") == 0) {
					printf("sound event\n");
					send(new_fd, "rec", 3, 0);
				} 
				else if (strcmp(buf, "disconnect") == 0) {
					break;
				}
				else {
					send(new_fd, "error", 5, 0);
				}
			}

			printf("Closing connection from %s\n", s);
			close(new_fd);
			exit(0);
		}
				
		
    }

    return 0;
}

