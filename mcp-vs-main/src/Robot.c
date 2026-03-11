// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Robot.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file

volatile uint16_t temp2 = 0;
volatile uint32_t previous_time = 0;

int main(void)
{
  serial0_init();
  milliseconds_init();

  char serialString[60] = {};
  uint16_t seconds_now = 0;

  cli();                 // closes the interrupt
  EICRA |= (1 << ISC31); //| (1<<ISC20);
  EIMSK |= (1 << INT3);
  sei(); // turns on the interrupt

  while (1)
  {
    if (milliseconds_now() / 1000 > seconds_now)
    {
      seconds_now = milliseconds_now() / 1000;

      sprintf(serialString, "\nFalling edges: %u", temp2);
      serial0_print_string(serialString);

      temp2 = 0;
    }
  }
  return (1);
}

ISR(INT3_vect) {
  uint32_t current_time = milliseconds_now();
  if (current_time > previous_time + 100) { // debounce period of 100ms
    temp2++;
    previous_time = current_time;
  }
}