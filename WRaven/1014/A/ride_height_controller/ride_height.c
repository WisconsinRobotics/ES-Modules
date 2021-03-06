#include "ride_height.h"

void pwm_init() {
    //Front PWM -> OC1A, OC1B
    //Back PWM -> OC0A, OC0B
    DDRB |= 0x02; 
	 DDRB |= 0x04;                             //PB1 and PD6 are the PWM pins OC
												//PB2andPD5aretheminuspins
    DDRD |= 0x40;
	DDRD |= 0x20;
    OCR0A = 128;                              //set PWM for 50% duty cycle
    OCR1A = 32768;
	    OCR0B = 0;                              //set PWM for 50% duty cycle
	    OCR1B = 0;
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
    uint8_t front_dir = (payload & front_dir_mask)>>2;
    uint8_t back_dir = payload & back_dir_mask;
	uint16_t front_speed = STOP_PWM;
	uint8_t back_speed = STOP_PWM;
    
    if (front_dir == ACTUATOR_DOWN) front_speed = FRONT_DOWN_PWM;
    else if (front_dir == ACTUATOR_UP) front_speed = FRONT_UP_PWM;
    else front_speed = STOP_PWM;

    if (back_dir == ACTUATOR_DOWN) back_speed = BACK_DOWN_PWM;
    else if (back_dir == ACTUATOR_UP) back_speed = BACK_UP_PWM;
    else back_speed = STOP_PWM;

     OCR0A = back_speed;
     OCR1A = front_speed;
}
