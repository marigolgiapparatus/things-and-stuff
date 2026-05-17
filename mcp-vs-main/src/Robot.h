//Example ATmega2560 Project
//File: ATmega2560Project.h
//Author: Robert Howie

#ifndef CONTROLLER_H_ //double inclusion guard
#define CONTROLLER_H_

//include standard libraries
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <util/delay.h>
#include <stdio.h>

//include header files
#include "serial.h" //minimal serial lib
#include "adc.h" //minimal adc lib
#include "milliseconds.h" //milliseconds timekeeping lib
#include "hd44780.h" //LCD lib

//constants
#define BUILD_DATE __TIME__ " " __DATE__"\n"

// Comment out this line to switch back to real XBee (serial2) operation
#define USB_SIM

#ifdef USB_SIM
  #define XBEE_AVAILABLE()  serial0_available()
  #define XBEE_GET(buf, n)  serial0_get_data(buf, n)
  #define XBEE_SEND(...)    serial0_write_bytes(__VA_ARGS__)
#else
  #define XBEE_AVAILABLE()  serial2_available()
  #define XBEE_GET(buf, n)  serial2_get_data(buf, n)
  #define XBEE_SEND(...)    serial2_write_bytes(__VA_ARGS__)
#endif

#endif /* ATMEGA2560_H_ */
