/* gpio_driver.h
    Interface for GPIO driver. The driver defines functions
    and macros to easily interface with the Raspberry Pi 2's
    GPIO pins.
*/
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
 
// Physical addresses
#define BCM2836_PERI_BASE       0x3F000000
#define GPIO_BASE               (BCM2836_PERI_BASE + 0x200000)
#define SYSTIME_BASE            (BCM2836_PERI_BASE + 0x003000)
#define BLOCK_SIZE              (4*1024)
 
//Constants and Macros
#define INPUT  0
#define OUTPUT 1
#define ALT0   4
#define ALT1   5
#define ALT2   6
#define ALT3   7
#define ALT4   3
#define ALT5   2
 
#define GPFSEL   ((volatile unsigned int *) (gpio + 0))
#define GPSET    ((volatile unsigned int *) (gpio + 7))
#define GPCLR    ((volatile unsigned int *) (gpio + 10))
#define GPLEV    ((volatile unsigned int *) (gpio + 13))
 
/* GPIO Interface */
 
/* pioInit
    Initialize memory mapped GPIO peripherals using /dev/mem */
void pioInit();

/* pioInitGpio
    Initialize memory mapped GPIO peripherals using dev/gpiomem */
void pioInitGpio();

/* pinMode
    Sets the function of a GPIO pin
    The function variable should take a value between 0 and 7:
        0 = Input
        1 = Output
        2-7 = Alternate Function */
void pinMode(const int pin, int function); 
 
/* digitalWrite
    Writes val to GPIO pin. Val should be 0 or 1  */
void digitalWrite(const int pin, int val);
 
/* digitalRead
    Reads from GPIO pin. Read value is returned as int */
int digitalRead(const int pin); 
