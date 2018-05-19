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

#define BACK_UP_PWM 8 //PWM length of 512 microseconds
#define BACK_DOWN_PWM 38 //PWM length of 2432 microseconds
#define FRONT_UP_PWM 33 //PWM length of 512 microseconds
#define FRONT_DOWN_PWM 157 //PWM length of 2432 microseconds
#define ACTUATOR_DOWN 2
#define ACTUATOR_UP 1
#define STOP_PWM 0
#define back_dir_mask 0x03
#define front_dir_mask 0x0C

void pwm_init();
void drive_actuators(uint8_t payload);
#endif
