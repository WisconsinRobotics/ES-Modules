#ifndef RIDE_HEIGHT_H
#define RIDE_HEIGHT_H
#include <avr/io.h>
#include <stdint.h>
#define FORWARD_PWM 8 //PWM length of 512 microseconds
#define BACKWARD_PWM 38 //PWM length of 2432 microseconds
#define STOP_PWM 0
void pwm_init();
void drive_actuators(uint8_t actuator, uint8_t speed);
#endif