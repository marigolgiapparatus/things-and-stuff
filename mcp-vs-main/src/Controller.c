// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Controller.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file

int main(void)
{
  adc_init();
  _delay_ms(20);

  DDRA = 0;
  PORTA = 0;
  PORTA |= (1<<PA7); // enable internal pullup resistor

  DDRC = 0xFF;//put PORTC into output mode
  PORTC = 0;

  uint16_t xVal = 0;
  uint16_t yVal = 0;

  while(1)//main loop
  {
    if (!(PINA)) {
      // if the switch is pressed
      // display X

      PORTC = 0 | (1<<(xVal>>7));
    }
    else {
      // if the switch is NOT pressed
      // display Y

      PORTC = 0 | (1<<(yVal>>7));
    }

    xVal = adc_read(7);
    yVal = adc_read(3);
  }
  return(1);
}