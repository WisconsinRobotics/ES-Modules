/* 
 * Ride Height Controller.c 
 * Created: 4/2/2018 7:44:07 PM 
 * Author : alexander
 * Modified by: Dakota
 */   

#define F_CPU 1000000UL
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "../../../common/i2c_packet.h"
#include <avr/io.h>
#include "ride_height.h"

#define PING_ON 0x01
#define PING_OFF 0x02
#define DRIVE_ACTUATORS 0x07

void init() {
    DDRD |= 1 << DDD0;
    DDRD |= 1 << DDD1;
    DDRD |= 1 << DDD2;
    i2c_init(0xAA);
    pwm_init();
    sei();
}

int main(void) {
    init();
    uint16_t alive_counter = 0;
    packet receivedPacket;

    while (1) {
        i2c_checkForPackets();
        if(i2c_hasPacket()) {
            //i2c_setReturnPacket(&returnPacket, 2);
            i2c_getPacket(&receivedPacket);
            PORTD |= 1<<PORTD2;
            _delay_ms(50);
            PORTD &= ~(1<<PORTD2);
            switch(receivedPacket.cmd) {
                case(DRIVE_ACTUATORS): 
                    drive_actuators(receivedPacket.buffer[0]);
                    break;
                case(PING_ON):
                    PORTD |= PORTD0;
                    break;
                case(PING_OFF):
                    PORTD &= ~PORTD0;
                    break;
            }
        }
        alive_counter++;
        if (alive_counter >= 5000) {
            PORTD ^= 1<< PORTD1;
            alive_counter = 0;
        }
    }
}
