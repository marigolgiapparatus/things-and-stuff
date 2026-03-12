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
  serial0_init();

  char serialString[60] = {};

  uint16_t sensor_value = 0;
  uint16_t voltage_equivalent = 0;
  uint16_t distance = 0;

  while (1) {
    sensor_value = adc_read(1);

    voltage_equivalent = sensor_value * (5000/1024);
    distance = 18900 / (voltage_equivalent - 292);

    sprintf(serialString, "\nDistance: %u cm Voltage: %u V", distance, voltage_equivalent);
    serial0_print_string(serialString);
  }
    
  return (1);
}