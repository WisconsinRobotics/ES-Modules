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

void drive_actuators(uint8_t actuators, uint8_t speed) {
    //pwm duty cycle. 0% -> 0, 100% -> 256
    //actuators is a bit field where the first 2 bits are a boolean
    //describing whether or not that actuator moves.
    switch (actuators) {
        //Front Motors
        case (1):
            OCR0A = speed;
            break;
        
        //Back Motors
        case(2):
            OCR1A = speed;
            break;

        //Both Front & Back
        case(3):
            OCR0A = speed;
            OCR1A = speed;
            break;
    }
}
