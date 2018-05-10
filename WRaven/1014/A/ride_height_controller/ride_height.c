#include "ride_height.h"

void pwm_init() {
    //Front PWM -> OC1A, OC1B
    //Back PWM -> OC0A, OC0B
    DDRB |= 0x02;                             //PB1 and PD6 are the PWM pins OC
    DDRD |= 0x40;
    OCR0A = 128;                              //set PWM for 50% duty cycle
    OCR1A = 32768;
    TCCR0A |= (1 << COM0A1);                  //set none-inverting mode
    TCCR1A |= (1 << COM0A1);                  //timer counter control register
    TCCR0A |= (1 << WGM01) | (1 << WGM00);    //set fast PWM Mode
    TCCR1A |= (1 << WGM01) | (1 << WGM00);
    TCCR0B |= 0x03;                           //set prescaler to 64 and starts PWM
    TCCR1B |= 0x02;                           //pwm period set to 0.016384 sec
}

void drive_actuators(uint8_t payload) {
    //pwm duty cycle. 0% -> 0, 100% -> 256
    //payload = direction_bit_1, direction_bit_0, actuator_bit_1, actuator_bit_0 
    uint8_t actuators = payload & actuator_mask;
    uint8_t direction = (payload & direction_mask) >> 2;  
	uint16_t front_speed = STOP_PWM;
	uint8_t back_speed = STOP_PWM;

    if (direction == DIR_FORWARD) { 
		front_speed = FRONT_FORWARD_PWM;
		back_speed = BACK_FORWARD_PWM;
	}
    else if (direction == DIR_BACKWARD){
		 front_speed = FRONT_BACKWARD_PWM;
		 back_speed = BACK_BACKWARD_PWM;
	}
    else {
		front_speed = STOP_PWM;
		back_speed = STOP_PWM;
	}
    if (actuators & (1<<BACK_ACTUATOR)) OCR0A = back_speed;
    if (actuators & (1<<FRONT_ACTUATOR)) OCR1A = front_speed;
}
