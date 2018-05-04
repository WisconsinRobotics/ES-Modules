/* 
 * Ride Height Controller.c 
 * Created: 4/2/2018 7:44:07 PM 
 * Author : alexander 
 */   
//launchpad >> this board >> motor controller via pwm //encoders  >>

#define F_CPU 1000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "../../../common/i2c_packet.h"
#include <avr/io.h>

#define PING_ON 0x01
#define PING_OFF 0x02
#define DriveActuator 0x07
#define FORWARD_PWM 8 //PWM length of 512 microseconds
#define BACKWARD_PWM 38 //PWM length of 2432 microseconds
#define STOP_PWM 0


void init();
void driveActuator(uint8_t actuator, int direction);


int main(void)
{
    //init();
    DDRD |= 1 << DDD0;
    DDRD |= 1 << DDD1;
    DDRD |= 1 << DDD2;
    i2c_init(0xAA);
    sei();
    uint16_t counter = 0;
    packet receivedPacket;
    packet returnPacket;
    returnPacket.buffer[0] = 0x00;
    returnPacket.buffer[1] = 0x00;
    uint16_t shouldFlip = 0x00;
    uint16_t didntGo = 0x00;
    while (1) 
    {
        i2c_setReturnPacket(&returnPacket, 2);
        i2c_checkForPackets();
        
        if(i2c_hasPacket())
        {
            PORTD ^= 1 << PORTD2;
            PORTD &= ~(1 << PORTD0);
            
            i2c_getPacket(&receivedPacket);
            switch(receivedPacket.cmd)
            {
                case(DriveActuator):
                    //driveActuator(receivedPacket.buffer[0], receivedPacket.buffer[1]);
                    shouldFlip = shouldFlip + 1;
                    returnPacket.buffer[0] = PORTD &= (1 << PORTD2);
                    returnPacket.buffer[1] = didntGo;
                    didntGo = 0x00;
                    break;

/*              case(PING_ON):
                    PORTD |= (1 << PORTD0);
                    break;

                case(PING_OFF):
                    PORTD &= ~(1 << PORTD0);
                    _delay_ms(5000);
                    break; */
            }


        }
        
        counter++;
        if (counter >= 1000){
            PORTD ^= 1<< PORTD1;
            counter = 0;
        }

        didntGo = didntGo + 1;

    }
}

/*
    PORTD |= 0x07;
    
    _delay_ms(5000);
    
    PORTD &= ~0x07;
    
    _delay_ms(5000);
*/



//8 is full reverse
//38 is full forward



//initializes the registers and i2c
void init()
{
    //i2c initialization
    

    //PWM initialization
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

void driveActuator(uint8_t actuator, int direction)
{
    //pwm duty cycle. 0% -> 0, 100% -> 256
    int pwm = 2;

    switch(direction)
    {
        //forward movement
        case(1):
        pwm = FORWARD_PWM;
        break;

        //stop movement
        case(0):
        pwm = STOP_PWM;
        break;

        //backward movement
        case(-1):
        pwm = BACKWARD_PWM;
        break;
    }
    
    if(pwm == 0 || pwm == 1 || pwm == -1)
    {
        switch (actuator)
        {
            //Front Motors
            case (1):
            OCR0A = pwm;
            break;
            
            //Back Motors
            case(2):
            OCR1A = pwm;
            break;

            //Both Front & Back
            case(3):
            OCR0A = pwm;
            OCR1A = pwm;
            break;
        }
    }
}

//things to do
/*
1. read commands from launchpad
2. read feedback from actuator
3. set pwm to of certain actuators to out, in or stop
4. initialize
*/
