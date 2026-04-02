// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Robot.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file

int main(void)
{
  //init adc
  adc_init();
  _delay_ms(20);

  // joystick & switch


  // init pwm

  // 0-1-2-3 -> 10-11-12-13

  TCCR1A = 0; TCCR1B = 0;

  TCCR1A |= (1<<COM1B1) | (1<<COM1A1);
  TCCR1B |= (1<<WGM13) | (1<<CS11);

  ICR1 = 20000; // sets frequency (needs calc) - sets top

  DDRB |= (1<<PB5)|(1<<PB6); 

  uint16_t compValue = 1600;
  uint16_t compValue2 = 1600;
  OCR1A = compValue;
  OCR1B = compValue2;

  // joystick val
  uint16_t xVal = 0;
  uint16_t yVal = 0;

  // servo output setup
  serial0_init();


  while (1) {
    xVal = adc_read(2);
    yVal = adc_read(1); // read joystick y val

    // map joystick val to pwm comp value
    compValue = xVal * 1.758 + 620; // map 620-2420

    compValue2 = yVal * 1.758 + 620; // map 620-2420

    OCR1A = compValue; // set pwm duty cycle

    OCR1B = compValue2; // set pwm duty cycle for second servo
  }

  return (1);
}