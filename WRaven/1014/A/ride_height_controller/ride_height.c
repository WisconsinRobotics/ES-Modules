#include "ride_height.h"

void pwm_init() {
    //Front PWM -> OC1A, OC1B
    //Back PWM -> OC0A, OC0B

    //PB1 and PD6 are the PWM pins OC
    DDRB |= 0x02;
    DDRD |= 0x40;

    // set PWM for 50% duty cycle
    OCR0A = 128;    //Output compare control register
    OCR1A = 128;

    // set none-inverting mode
    TCCR0A |= (1 << COM0A1);    //timer counter control register
    TCCR1A |= (1 << COM0A1);

    // set fast PWM Mode
    TCCR0A |= (1 << WGM01) | (1 << WGM00);
    TCCR1A |= (1 << WGM01) | (1 << WGM00);
    
    // set prescaler to 64 and starts PWM
    //pwm period set to 0.016384 sec
    TCCR0B |= 0x03;
    TCCR1B |= 0x03;
}

void drive_actuators(uint8_t payload) {
    //pwm duty cycle. 0% -> 0, 100% -> 256
    uint8_t actuators = payload & actuator_mask;
    uint8_t direction = (payload & direction_mask) >> 2;  

    if (direction == DIR_FORWARD) speed = FORWARD_PWM;
    else if (direction == DIR_BACKWARD) speed = BACKWARD_PWM;
    else speed = STOP_PWM;

    if (actuators == BACK) OCR0A = speed;
    else if (acutators == FRONT) OCR1A = speed;
    else if (actuators == BOTH) {
        ORC0A = speed;
        ORC1A = speed;
    }
}
