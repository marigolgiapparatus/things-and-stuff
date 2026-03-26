// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Robot.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file
volatile uint32_t milliseconds = 0;
char serialString[100] = {};

int main(void)
{
  // set up whatever
  serial0_init();

  // timer for updates:
  TCCR1A = 0; // disable pin output
  TCCR1B = (1<<CS12)|(1<<CS10)|(1<<WGM12); // CTC mode, prescaler 1024
  TIMSK1 = (1<<OCIE1A);
  OCR1A = 776; 

  // internal clock
  TCCR5A = 0;
  TCCR5B = (1<<WGM52);
  TCNT5 = 0; // reset timer
  OCR5A = 15999; // 1ms at 16MHz
  TIMSK5 |= (1<<OCIE5A); // output compare
  TCCR5B |= (1<<CS50); // prescaler 1


  // button set up
  cli(); // closes the interrupt
  EICRA |= (1 << ISC31); //| (1 << ISC30);
  EIMSK |= (1 << INT3);

  EICRA |= (1<<ISC21); //| (1<<ISC20);
  EIMSK |= (1<<INT2);

  sei(); // turns on the interrupt

  // set up timer

  while (1) {
    // set up main code

    // set up button interrupt
  }

  return (1);
}

// button 1 (pause) interrupt code
ISR(INT2_vect) {
  // button pauses until its pressed again
  if (TCCR5B & (1<<CS50)) {
    TCCR5B &= ~(1<<CS50); // stop timer
  } else {
    TCCR5B |= (1<<CS50); // start timer
  } 

  return;
}
// button 2 (reset) interrupt code
ISR(INT3_vect) {
  milliseconds = 0;
  return;
}

uint32_t getMilliseconds(void) {
  uint32_t m;
  uint8_t oldSREG = SREG;

  cli();
  m = milliseconds;
  SREG = oldSREG;

  return m;
}

ISR(TIMER5_COMPA_vect) {
  milliseconds++;
}

ISR(TIMER1_COMPA_vect) {
  volatile uint32_t millis = milliseconds;
  uint32_t mins = millis / 3600000;
  uint32_t secs = (millis % 60000) / 1000;
  uint32_t ms = millis % 1000;
  sprintf(serialString, "%lumin %lusec %lums total: %u\n", mins, secs, ms, millis); 
  // sprintf(serialString, "%lu sec %u ms\n", secs, ms);

  serial0_print_string(serialString);
}
