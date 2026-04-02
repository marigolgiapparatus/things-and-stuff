// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Robot.h"
#include "../lib/adc/adc.h"

// static function prototypes, functions only called in this file

int main(void)
{
  serial0_init();
  serial2_init();

  TCCR1A = 0; TCCR1B = 0;
  TCCR1A |= (1<<COM1B1) | (1<<COM1A1);
  TCCR1B |= (1<<WGM13) | (1<<CS11);
  ICR1 = 20000;
  DDRB |= (1<<PB5) | (1<<PB6);

  OCR1A = 1500;

  milliseconds_init();
  adc_init();

  char serialString[100] = {};
  uint16_t current_ms = 0;
  uint16_t last_send_ms = 0;
  uint16_t front_sensor_value;
  uint16_t distance;
  uint8_t rx_data[6];

  while (1) {
    current_ms = milliseconds_now();

    front_sensor_value = adc_read(1) / 4;
    front_sensor_value = (front_sensor_value > 253) ? 253 : front_sensor_value;

    if ( serial2_available() ) {
      serial2_get_data(rx_data, 2);

      sprintf(serialString, "%u %u\n", rx_data[0], rx_data[1]);
      serial0_print_string(serialString);

      OCR1A = rx_data[0] * 4 * 1.76 + 620;
    }

    if ( ( current_ms - last_send_ms) >= 50 ) {
      serial2_write_bytes(3, front_sensor_value, 1, 2);
      last_send_ms = current_ms;
    }
    //if ( serial2_available )
  }

  return (1);
}