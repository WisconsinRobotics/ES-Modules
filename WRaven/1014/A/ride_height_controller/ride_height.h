#ifndef RIDE_HEIGHT_H
#define RIDE_HEIGHT_H
#include <avr/io.h>
#include <stdint.h>
#define CMD_PING_ON 0x01
#define CMD_PING_OFF 0x02
#define CMD_DRIVE_ACTUATORS 0x07
#define LED_FLASH_TIME 50
#define I2C_ADDR 0x0A
#define LED_PING_ON (PORTD |= (1<<PORTD0))
#define LED_PING_OFF (PORTD &= ~(1<<PORTD0))
#define LED_CMD_ON (PORTD |= (1<<PORTD2))
#define LED_CMD_OFF (PORTD &= ~(1<<PORTD2))
#define LED_ALIVE_TOGGLE (PORTD ^= (1<<PORTD1))
#define LED_ALIVE_FREQ 5000

#define BACK_FORWARD_PWM 38 //PWM length of 512 microseconds
#define BACK_BACKWARD_PWM 8 //PWM length of 2432 microseconds
#define FRONT_FORWARD_PWM 157 //PWM length of 512 microseconds
#define FRONT_BACKWARD_PWM 33 //PWM length of 2432 microseconds
#define STOP_PWM 0
#define DIR_STOP 0
#define DIR_FORWARD 2
#define DIR_BACKWARD 1
#define BACK_ACTUATOR 0
#define FRONT_ACTUATOR 1
#define actuator_mask 0x03
#define  direction_mask 0x0C
void pwm_init();
void drive_actuators(uint8_t payload);
#endif
