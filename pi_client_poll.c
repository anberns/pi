/******************************************************************************
 * pi_client_poll.c : Connects to pi_server, uses OpenMP to concurrently monitor 
 * a variety of sensor inputs, communicating sensor events to server for 
 * processing.
 *
 * to enable ad-hoc, uncomment changes to /etc/network/interfaces
 * to enable auto startup and shutdown, uncomment lines in ~/superscript
 * to remove superscript, comment out last line in .bashrc
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
#include <poll.h>

#define MAXDATASIZE 600
#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

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

// gpio file polling functions from developer.ridgerun.com

int gpio_export(unsigned int gpio)
{
        int fd, len;
        char buf[MAX_BUF];
 
        fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
        if (fd < 0) {
                perror("gpio/export");
                return fd;
        }
 
        len = snprintf(buf, sizeof(buf), "%d", gpio);
        write(fd, buf, len);
        close(fd);
 
        return 0;
}

int gpio_unexport(unsigned int gpio)
{
        int fd, len;
        char buf[MAX_BUF];
 
        fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
        if (fd < 0) {
                perror("gpio/export");
                return fd;
        }
 
        len = snprintf(buf, sizeof(buf), "%d", gpio);
        write(fd, buf, len);
        close(fd);
        return 0;
}

int gpio_set_dir(int gpio, unsigned int out_flag)
{
        int fd, len;
        char buf[MAX_BUF];
 
        len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
 
        fd = open(buf, O_WRONLY);
        if (fd < 0) {
                perror("gpio/direction");
                return fd;
        }
 
        if (out_flag == 1)
                write(fd, "out", 4);
        else
                write(fd, "in", 3);
 
        close(fd);
        return 0;
}

int gpio_set_value(int gpio, unsigned int value)
{
        int fd, len;
        char buf[MAX_BUF];
 
        len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
        fd = open(buf, O_WRONLY);
        if (fd < 0) {
                perror("gpio/set-value");
                return fd;
        }
 
        if (value)
                write(fd, "1", 2);
        else
                write(fd, "0", 2);
 
        close(fd);
        return 0;
}

int gpio_get_value(int gpio, unsigned int *value)
{
        int fd, len;
        char buf[MAX_BUF];
        char ch;

        len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
        fd = open(buf, O_RDONLY);
        if (fd < 0) {
                perror("gpio/get-value");
                return fd;
        }
 
        read(fd, &ch, 1);

        if (ch != '0') {
                *value = 1;
        } else {
                *value = 0;
        }
 
        close(fd);
        return 0;
}

int gpio_set_edge(int gpio, char *edge)
{
        int fd, len;
        char buf[MAX_BUF];

        len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
 
        fd = open(buf, O_WRONLY);
        if (fd < 0) {
                perror("gpio/set-edge");
                return fd;
        }
 
        write(fd, edge, strlen(edge) + 1); 
        close(fd);
        return 0;
}

int gpio_fd_open(int gpio)
{
        int fd, len;
        char buf[MAX_BUF];

        len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
 
        fd = open(buf, O_RDONLY | O_NONBLOCK );
        if (fd < 0) {
                perror("gpio/fd_open");
        }
        return fd;
}

int gpio_fd_close(int fd)
{
        return close(fd);
}

void closeConnection(int sockfd) {
    close(sockfd);
}

void sendEvent(int sockfd, char *event) {
	// send message to server
    int len = strlen(event);
    int bytes_sent = send(sockfd, event, len, 0);
}

int main(int argc, char *argv[])
{
	char *sound = "sound";
	char *disconnect = "disconnect";
	char *smoke = "smoke";
	char *motion = "motion";

	int end = 0; // shared varible to kill all threads before shutdown

	// assign gpio pins
	const int red_led = 21; // smoke
	const int blue_led = 5; // power
	const int green_led = 16; // motion
	const int yellow_led = 20; // sound
	const int sound_sensor = 12;
	const int touch_sensor = 19;
	const int motion_sensor = 13;
	const int smoke_sensor = 6;

	// array for initializing pins by type
	const int led_array[] = {21, 5, 16, 20};
	const int sensor_array[] = {12, 19, 13, 6};

	int red_fd, blue_fd, green_fd, yellow_fd, sound_fd, touch_fd, motion_fd, smoke_fd;
	
	// set up pin files
	int i;
	for (i = 0; i < 4; ++i) {
		
		gpio_export(led_array[i]);
	}

	for (i = 0; i < 4; ++i) {
		
		gpio_export(sensor_array[i]);
	}

	for (i = 0; i < 4; ++i) {
		
		gpio_set_dir(led_array[i], 1); // 1 is out
	}

	for (i = 0; i < 3; ++i) {
		
		gpio_set_dir(sensor_array[i], 0); // 0 is in
		gpio_set_edge(sensor_array[i], "rising");
	}

	// smoke is both, detection and end of detection
	gpio_set_dir(sensor_array[3], 0); // 0 is in
	gpio_set_edge(sensor_array[3], "both");

	red_fd = gpio_fd_open(red_led);
	blue_fd = gpio_fd_open(blue_led);
	green_fd = gpio_fd_open(green_led);
	yellow_fd = gpio_fd_open(yellow_led);
	sound_fd = gpio_fd_open(sound_sensor);
	touch_fd = gpio_fd_open(touch_sensor);
	motion_fd = gpio_fd_open(motion_sensor);
	smoke_fd = gpio_fd_open(smoke_sensor);

	// used for polling sensor gpio files
	struct pollfd pfd[1];
	int nfds = 1;
	int gpio_fd, timeout, rc;
	char *buf[MAX_BUF];
	int len;

	// reset leds if on
	gpio_set_value(blue_led, 0);
	gpio_set_value(red_led, 0);

	// attempt to connect to specified server and listening port
	// exit if unable after 10 tries
	int sockfd = -1;
	int attempts = 0;
	do {
		sockfd = estConnection(argv[1], argv[2]);
		if (sockfd < 0) {
			gpio_set_value(blue_led, 1);
			sleep(1);
			gpio_set_value(blue_led, 0);
			sleep(1);
			++attempts;
		}
		else {
			gpio_set_value(blue_led, 1);
		}

	} while (sockfd < 0 && attempts < 10);

	if (sockfd < 0) {
		exit(1);
	}

	// OpenMP sections used for concurrent sensor monitoring
	omp_lock_t messageLock; // only one thread sends message to server at a time
	omp_init_lock(&messageLock);
	omp_set_num_threads(6);
	#pragma omp parallel sections default(none), private(pfd, rc, buf), shared(nfds, end, sockfd, smoke, sound, motion, red_fd, blue_fd, green_fd, yellow_fd, sound_fd, touch_fd, motion_fd, smoke_fd)
	{
		// temp sensor thread
		#pragma omp section
		{
			// variables for temp sensor, help from bradsrpi.blogspot.com
			char buf2[256];
			char tmpData[6];
			char path[] = "/sys/bus/w1/devices/28-051693b4daff/w1_slave";
			ssize_t numRead;
			float temp;
			char message[13];
			memset(&message, sizeof message, '\0');
			char oldMessage[13];
			memset(&oldMessage, sizeof oldMessage, '\0');

			int fd;

			// check temp file every 2 seconds, send temp if it has changed
			while (!end) {
				fd = open(path, O_RDONLY);
				while ((numRead = read(fd, buf2, 256)) > 0) {
					strncpy(tmpData, strstr(buf2, "t=") +2, 5); 
					temp = strtof(tmpData, NULL);
					temp = (temp / 1000) * (9/5) + 32;
					sprintf(message, "temp %.1f F", temp);
				}

				if (strcmp(message, oldMessage) != 0) {
					omp_set_lock(&messageLock);
					sendEvent(sockfd, message);
					omp_unset_lock(&messageLock);
					strcpy(oldMessage, message);
				}
				close(fd);
				sleep(2);
			}
		}

		// sound sensor polling thread
		#pragma omp section
		{
			while (!end) {
				pfd[0].fd = sound_fd;
				pfd[0].events = POLLPRI;
				lseek(sound_fd, 0, SEEK_SET);
				read(sound_fd, buf, sizeof(buf));
				rc = poll(pfd, nfds, POLL_TIMEOUT);

				if (pfd[0].revents & POLLPRI) {
					lseek(sound_fd, 0, SEEK_SET);
					read(sound_fd, buf, sizeof(buf));
					gpio_set_value(yellow_led, 1);
					omp_set_lock(&messageLock);
					sendEvent(sockfd, sound);
					omp_unset_lock(&messageLock);
					sleep(1);
					gpio_set_value(yellow_led, 0);
				}

			}
		}

		// motion sensor polling thread
		#pragma omp section
		{
			while (!end) {
				pfd[0].fd = motion_fd;
				pfd[0].events = POLLPRI;
				lseek(motion_fd, 0, SEEK_SET);
				read(motion_fd, buf, sizeof(buf));
				rc = poll(pfd, nfds, POLL_TIMEOUT);

				if (pfd[0].revents & POLLPRI) {
					lseek(motion_fd, 0, SEEK_SET);
					read(motion_fd, buf, sizeof(buf));
					gpio_set_value(green_led, 1);
					omp_set_lock(&messageLock);
					sendEvent(sockfd, motion);
					omp_unset_lock(&messageLock);
					sleep(1);
					gpio_set_value(green_led, 0);
				}
			}
		}

		// smoke sensor polling thread
		#pragma omp section
		{
			int val;
			while (!end) {
				pfd[0].fd = smoke_fd;
				pfd[0].events = POLLPRI;
				lseek(smoke_fd, 0, SEEK_SET);
				read(smoke_fd, buf, sizeof(buf));
				rc = poll(pfd, nfds, POLL_TIMEOUT);

				if (pfd[0].revents & POLLPRI) {
					lseek(smoke_fd, 0, SEEK_SET);
					read(smoke_fd, buf, sizeof(buf));

					gpio_get_value(red_led, &val);

					if (val) {
						gpio_set_value(red_led, 0);
					}
					else {
						gpio_set_value(red_led, 1);
					}

					// send message to server for each edge change
					omp_set_lock(&messageLock);
					sendEvent(sockfd, smoke);
					omp_unset_lock(&messageLock);
					sleep(1);
					
				}			
			}
		}

		// thread for button that ends program
		#pragma omp section
		{ 
			while(!end) {
				pfd[0].fd = touch_fd;
				pfd[0].events = POLLPRI;
				lseek(touch_fd, 0, SEEK_SET);
				read(touch_fd, buf, sizeof(buf));
				rc = poll(pfd, nfds, POLL_TIMEOUT);

				if (pfd[0].revents & POLLPRI) {
					end = 1;
				}
			}
		}
	}

	//flash blue led indicating shutdown
	gpio_set_value(blue_led, 0);
	sleep(1);
	gpio_set_value(blue_led, 1);
	sleep(1);
	gpio_set_value(blue_led, 0);

	// disconnect from server
	omp_set_lock(&messageLock);
	sendEvent(sockfd, disconnect);
	omp_unset_lock(&messageLock);
	close(sockfd);
	
	// close fd's
	red_fd = gpio_fd_close(red_led);
	blue_fd = gpio_fd_close(blue_led);
	green_fd = gpio_fd_close(green_led);
	yellow_fd = gpio_fd_close(yellow_led);
	sound_fd = gpio_fd_close(sound_sensor);
	touch_fd = gpio_fd_close(touch_sensor);
	motion_fd = gpio_fd_close(motion_sensor);
	smoke_fd = gpio_fd_close(smoke_sensor);

	// unexport gpio's
	for (i = 0; i < 4; ++i) {
		
		gpio_unexport(led_array[i]);
	}

	for (i = 0; i < 4; ++i) {
		
		gpio_unexport(sensor_array[i]);
	}
	return 0;
}
