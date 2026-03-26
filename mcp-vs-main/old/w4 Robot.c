// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Robot.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file

int main(void)
{
  DDRA = 0xFF;
  PORTA = 0;
  PORTA = (1<<PIN1);
  while(1) 
  {
    PORTA ^= (1<<PIN1);
    _delay_ms(3000);
  }
}