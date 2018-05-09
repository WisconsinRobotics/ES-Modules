#ifndef RIDE_HEIGHT_H
#define RIDE_HEIGHT_H
#include <avr/io.h>
#include <stdint.h>
#define FORWARD_PWM 8 //PWM length of 512 microseconds
#define BACKWARD_PWM 38 //PWM length of 2432 microseconds
#define STOP_PWM 0
#define DIR_STOP 0
#define DIR_FORWARD 1
#define DIR_BACKWARD 2
#define BACK_ACTUATOR 0
#define FRONT_ACTUATOR 1
const uint8_t actuator_mask = 0x03;
const uint8_t direction_mask = 0x0C;
void pwm_init();
void drive_actuators(uint8_t actuator, uint8_t speed);
#endif