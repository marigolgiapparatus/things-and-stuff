// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Robot.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file

int main(void)
{
  adc_init();
  _delay_ms(20);

  DDRC = 0xFF;//put PORTC into output mode
  PORTC = 0;

  uint16_t voltageOverFixed = 0;

  while(1)//main loop
  {
    PORTC = 0 | (1<<((((voltageOverFixed-245)*128/59))>>7));

    voltageOverFixed = adc_read(0);
  }
  return(1);
}