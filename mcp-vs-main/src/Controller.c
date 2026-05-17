// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Controller.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file
//volatile uint32_t milliseconds = 0;

void setup() {
  adc_init();
  lcd_init();
  serial0_init();
  serial2_init();
  serial3_init();

  milliseconds_init();
  _delay_ms(20);

  // read D21 / PD0 to determine control mode
  DDRD &= ~(1 << PD0); // set PD0 as input
  PORTD |= (1 << PD0); // enable pull-up resistor on PD0
  EICRA |= (1 << ISC01); // trigger on falling edge
  EIMSK |= (1 << INT0); // enable external interrupt INT0
  sei(); // enable global interrupts
}

volatile bool auto_mode = false;

int main(void)
{
  setup();

  uint32_t last_ms = 0;
  uint8_t rx_data[6];
  char serialString[100] = {};
  char lcd_string[17];

  lcd_clrscr();
  lcd_puts("hello world");
  _delay_ms(1000);
  lcd_clrscr();

  while (1) {
    uint32_t current_ms = milliseconds_now();

    lcd_goto(0x40); // 2nd line
    lcd_puts(auto_mode ? "AUTO MODE" : "MANUAL MODE");

    if (serial2_available()) {
      serial2_get_data(rx_data, 6);

      uint16_t front_distance = (rx_data[0]);
      uint16_t left_distance = (rx_data[1]);
      uint16_t right_distance = (rx_data[2]); 

      //uint8_t leftPercent = (leftLight * 100UL) / 255;
      //uint8_t rightPercent = (rightLight * 100UL) / 255;

      // Do something with the received data
      sprintf(lcd_string, "%2ucm %2ucm %2ucm", front_distance, left_distance, right_distance);
      lcd_goto(0x00);
      lcd_puts(lcd_string);
    }

    if (( current_ms - last_ms) >= 50 ) {
      uint16_t xVal = (adc_read(0) >> 2) & 0xFD;
      uint16_t yVal = (adc_read(1) >> 2) & 0xFD;

      // 2nd joystick
      // uint16_t xVal2 = (adc_read(15) >> 2) & 0xFD;
      uint16_t yVal2 = (adc_read(14) >> 2) & 0xFD;

      serial2_write_bytes(4, xVal, yVal, yVal2, (uint8_t)auto_mode);

      last_ms = current_ms;
    }
  }

  return (1);
}

ISR (INT0_vect) {
  static uint32_t last_ms = 0;
  uint32_t current_ms = milliseconds_now();

  if (current_ms - last_ms > 200) {
    auto_mode = !auto_mode; // toggle control mode
    last_ms = current_ms;
  }
}