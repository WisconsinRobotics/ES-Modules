#ifndef RIDE_HEIGHT_H
#define RIDE_HEIGHT_H
#include <avr/io.h>
#include <stdint.h>
#define BACK_FORWARD_PWM 38 //PWM length of 512 microseconds
#define BACK_BACKWARD_PWM 8 //PWM length of 2432 microseconds
#define FRONT_FORWARD_PWM 157 //PWM length of 512 microseconds
#define FRONT_BACKWARD_PWM 31 //PWM length of 2432 microseconds
#define STOP_PWM 0
#define DIR_STOP 0
#define DIR_FORWARD 1
#define DIR_BACKWARD 2
#define BACK_ACTUATOR 0
#define FRONT_ACTUATOR 1
#define actuator_mask 0x03
#define  direction_mask 0x0C
void pwm_init();
void drive_actuators(uint8_t payload);
#endif