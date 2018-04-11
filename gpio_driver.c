/* gpio_driver.c
    Implementation for GPIO driver. The driver defines functions
    and macros to easily interface with the Raspberry Pi 2's 
    GPIO pins.
*/
 
#include "gpio_driver.h"
 
// Pointers that will be memory mapped when pioInit() is called
volatile unsigned int *gpio;    //pointer to base of gpio
 
// Access memory of the BCM2835
void pioInit() {
	int  mem_fd;
	void *reg_map;
 
	// /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
	if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
	      printf("can't open /dev/mem \n");
	      exit(-1);
	}
 
	reg_map = mmap(
            NULL,             //Address at which to start local mapping (null means don't-care)
            BLOCK_SIZE,       //Size of mapped memory block
            PROT_READ|PROT_WRITE,// Enable both reading and writing to the mapped memory
            MAP_SHARED,       // This program does not have exclusive access to this memory
            mem_fd,           // Map to /dev/mem
            GPIO_BASE);       // Offset to GPIO peripheral
 
	if (reg_map == MAP_FAILED) {
            printf("gpio mmap error %d\n", (int)reg_map);
            close(mem_fd);
            exit(-1);
        }
 
	gpio = (volatile unsigned *)reg_map;
}
 
// Access gpio memory of the BCM2835
void pioInitGpio() {
	int  mem_fd;
	void *reg_map;
 
	if ((mem_fd = open("/dev/gpiomem", O_RDWR|O_SYNC) ) < 0) {
	      printf("can't open /dev/gpiomem \n");
	      exit(-1);
	}
 
	reg_map = mmap(
            NULL,             //Address at which to start local mapping (null means don't-care)
            BLOCK_SIZE,       //Size of mapped memory block
            PROT_READ|PROT_WRITE,// Enable both reading and writing to the mapped memory
            MAP_SHARED,       // This program does not have exclusive access to this memory
            mem_fd,           // Map to /dev/mem
            GPIO_BASE);       // Offset to GPIO peripheral
 
	if (reg_map == MAP_FAILED) {
            printf("gpio mmap error %d\n", (int)reg_map);
            close(mem_fd);
            exit(-1);
        }
 
	gpio = (volatile unsigned *)reg_map;
}
//Sets the function of a GPIO pin
void pinMode(const int pin, int function) {
    //Determine FPSEL register offset and bit shift
    unsigned offset, shift;
    offset = pin / 10;
    shift = (pin % 10) * 3;
 
    //Clear the bits in FSELn
    GPFSEL[offset] &= ~(0b111 << shift);
 
    //Set the bits to the appropriate function
    GPFSEL[offset] |= (function << shift);
}
 
//Write val to GPIO pin
void digitalWrite(const int pin, int val) {
    //Determine the register offset, which is the offset for the
    //GPSET and GPCLR registers based on the pin number
    int offset = (pin < 32) ? 0 : 1;
 
    //Set the pin value
    if (val) {
        GPSET[offset] = 0x1 << (pin % 32);
    } else {
        GPCLR[offset] = 0x1 << (pin % 32);
    }
}
 
//Read from GPIO pin
int digitalRead(const int pin) {
    //To read, perform a bitwise AND on the corresponding
    //bit in the GPLEV register
    if (pin < 32) {
        return GPLEV[0] &= (0x1 << (pin % 32));
    } else {
        return GPLEV[1] &= (0x1 << (pin % 32));
    }
}
