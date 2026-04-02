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
      uint16_t leftLight = (rx_data[1]);
      uint16_t rightLight = (rx_data[2]); 

      uint8_t leftPercent = (leftLight * 100UL) / 255;
      uint8_t rightPercent = (rightLight * 100UL) / 255;

      uint32_t range_voltage = distance * (5000/256);
      uint32_t range_cm = 0;
      if (range_voltage > 0) {
        range_cm = 18900 / (range_voltage - 292);
      }

      // Do something with the received data
      sprintf(lcd_string, "L%3uR%3uD%3u", leftPercent, rightPercent, range_cm);
      lcd_goto(0x00);

      lcd_puts(lcd_string);
    }

    if (( current_ms - last_ms) >= 50 ) {
      uint16_t xVal = (adc_read(0) >> 2) & 0xFD;
      uint16_t yVal = (adc_read(1) >> 2) & 0xFD;

      serial2_write_bytes(2, xVal, yVal);

      last_ms = current_ms;
    }
  }

  return (1);
}