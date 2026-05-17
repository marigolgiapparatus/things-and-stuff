// Example ATmega2560 Project
// File: ATmega2560Project.c
// An example file for second year mechatronics project

// include this .c file's header file
#include "Robot.h"
#include "../lib/adc/adc.h"

typedef enum { MODE_AUTO, MODE_MANUAL } ControlMode;

// static function prototypes, functions only called in this file

int8_t bound_sensors(int8_t sensor_value, int8_t low_bound, int8_t high_bound) {
  if (sensor_value < low_bound) {
    sensor_value = low_bound;
  } else if (sensor_value > high_bound) {
    sensor_value = high_bound;
  }

  return sensor_value;
}

void setup(void) {
  serial0_init();
  serial2_init();

  // motor setup
  TCCR3A = 0; TCCR3B = 0;
  TCCR3A |= (1<<COM3B1) | (1<<COM3A1);
  TCCR3B |= (1<<WGM33) | (1<<CS31);
  ICR3 = 20000;
  DDRE |= (1 << PE3) | (1 << PE4); // Set OC3A and OC3B as outputs for motor PWM

  // timer setup
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1A |= (1 << COM1A1);
  TCCR1B |= (1 << WGM13) | (1 << CS11);
  ICR1 = 20000;
  DDRB |= (1 << PB5) | (1 << PB6);
  OCR1A = 1500;

  // battery setup
  DDRB |= (1 << PB7); // set PB7 as output for battery voltage reading

  milliseconds_init();
  adc_init();
}

void motor_magic(uint16_t x, uint16_t y) {
  // update motor speed and direction based on x and y values from controller

  int16_t yVal = (253 - x) - 126; // -126 to 127
  int16_t xVal = (y) - 126; // -126 to 127

  int16_t left_raw = (xVal + yVal);
  int16_t right_raw = (-xVal + yVal);

  // float scale = max_float(fabs(left_raw), fabs(right_raw), 1.0f); // code not used

  int16_t lm = left_raw;
  int16_t rm = right_raw;

  OCR3A = (int32_t)abs(lm) * 10000 / 126; // lm speed from magnitude of lm
  OCR3B = (int32_t)abs(rm) * 10000 / 126; // rm speed from magnitude of rm

  if (lm >= 0) // if lm is positive
  {
    // set direction forwards
    PORTA |= (1 << PA0);
    PORTA &= ~(1 << PA1);
  }
  else
  {
    // set direction reverse
    PORTA &= ~(1 << PA0);
    PORTA |= (1 << PA1);
  }

  if (rm >= 0) // if rm is positive
  {
    // set direction forwards
    PORTA |= (1 << PA2);
    PORTA &= ~(1 << PA3);
  }
  else
  {
    // set direction reverse
    PORTA &= ~(1 << PA2);
    PORTA |= (1 << PA3);
  }
  
  DDRA |= (1<<DDA0)|(1<<DDA1)|(1<<DDA2)|(1<<DDA3); //put A0-A3 into low impedance output mode
}

// void gripper_motor(uint16_t xVal, uint16_t yVal) {
//   comp
// }

void low_battery_check(uint16_t battery_adc) {
  // 7/2 = 3.5 -> 3.5/5 * 1024 = 716.8 , so 716 V \approx 7V;
  if (battery_adc < 716) { // if battery voltage is below 7V
    // do something
    PORTB |= (1 << PB7); // set PB7 high to indicate low battery
  }
  else {
    PORTB &= ~(1 << PB7); // set PB7 low to indicate sufficient battery
  }
}

float max_float(float a, float b, float c) {
  if (a > b && a > c) return a;
  if (b > a && b > c) return b;
  return c;
}

int main(void)
{
  setup();

  char serialString[100] = {};
  uint16_t current_ms = 0;
  uint16_t last_send_ms = 0;
  uint16_t front_sensor_value;
  uint16_t right_sensor_value;
  uint16_t left_sensor_value;
  uint16_t battery_adc;
  uint16_t distance;
  uint8_t rx_data[6];

  while (1) {
    current_ms = milliseconds_now();

    battery_adc = adc_read(0);
    low_battery_check(battery_adc); 

    // Find sensor values -> convert to centimeters -> bound to reasonable range
    front_sensor_value = bound_sensors(34 / adc_read(1) - 6, 10, 80);
    left_sensor_value = bound_sensors(12.5 / adc_read(2), 4, 30);
    right_sensor_value = bound_sensors(12.5 / adc_read(3), 4, 30);

    sprintf(serialString, "Sensors: %2ucm %2ucm %2ucm\n", front_sensor_value, left_sensor_value, right_sensor_value); // remove later
    serial0_print_string(serialString);
    
    // if (serial2_available)

    if (XBEE_AVAILABLE()) {
      XBEE_GET(rx_data, 4);

      uint8_t auto_mode = rx_data[3]; // 0 for manual, 1 for auto

      if (auto_mode) {
        // auto mode code here
      } else {
        motor_magic(rx_data[0], rx_data[1]); // set motor speeds and directions based on raw input
        OCR1A = rx_data[2] * 4 * 1.76 + 620; // need to make it delta
      }

      sprintf(serialString, "%d %d\n", OCR3A, OCR3B); // how fast motors are rotating
      serial0_print_string(serialString);
    }

    if ( ( current_ms - last_send_ms) >= 50 ) {
      XBEE_SEND(3, front_sensor_value, right_sensor_value, left_sensor_value);
      last_send_ms = current_ms;
    }
    //if ( serial2_available )

  }

  return (1);
}