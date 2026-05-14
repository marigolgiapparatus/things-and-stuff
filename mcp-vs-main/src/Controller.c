// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Controller.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file
//volatile uint32_t milliseconds = 0;

int main(void)
{
  adc_init();
  lcd_init();
  serial0_init();
  serial2_init();
  serial3_init();

  milliseconds_init();
  _delay_ms(20);

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

    if (serial2_available()) {
      serial2_get_data(rx_data, 3);

      uint16_t distance = (rx_data[0]);
      uint16_t right_distance = (rx_data[1]);
      uint16_t left_distance = (rx_data[2]); 

      //uint8_t leftPercent = (leftLight * 100UL) / 255;
      //uint8_t rightPercent = (rightLight * 100UL) / 255;

      uint32_t range_voltage = distance * (5000/256);
      uint32_t range_cm = 0;
      if (range_voltage > 0) {
        range_cm = 18900 / (range_voltage - 292);
      }

      // Do something with the received data
      sprintf(lcd_string, "L%3uR%3uD%3u", range_cm, 18900 / (right_distance * (5000/256)-292), 18900 / (left_distance * (5000/256)-292));
      lcd_goto(0x00);

      lcd_puts(lcd_string);
    }

    if (( current_ms - last_ms) >= 50 ) {
      uint16_t xVal = (adc_read(0) >> 2) & 0xFD;
      uint16_t yVal = (adc_read(1) >> 2) & 0xFD;

      // 2nd joystick
      // uint16_t xVal2 = (adc_read(15) >> 2) & 0xFD;
      uint16_t yVal2 = (adc_read(14) >> 2) & 0xFD;

      serial2_write_bytes(3, xVal, yVal, yVal2);

      last_ms = current_ms;
    }
  }

  return (1);
}