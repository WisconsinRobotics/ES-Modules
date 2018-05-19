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

void init() {
    DDRD |= 1 << DDD0;
    DDRD |= 1 << DDD1;
    DDRD |= 1 << DDD2;
    i2c_init(I2C_ADDR);
    pwm_init();
    sei();
    drive_actuators(0);
}

void display_cmd(){
    LED_CMD_ON;
    _delay_ms(LED_FLASH_TIME);
    LED_CMD_OFF;
}

void display_ping(){
    LED_PING_ON;
    _delay_ms(LED_FLASH_TIME);
    LED_PING_OFF;
}

void display_alive(){
    static uint16_t alive_counter = 0;
    if (alive_counter >= LED_ALIVE_FREQ) {
        LED_ALIVE_TOGGLE;
        alive_counter = 0;
    }
    alive_counter++;
}

int main(void) {
    init();
    packet receivedPacket;
    while (1) {
        display_alive();
        i2c_checkForPackets();
        if(i2c_hasPacket()) {
            display_cmd();
            i2c_getPacket(&receivedPacket);
            switch(receivedPacket.cmd) {
                case(CMD_DRIVE_ACTUATORS):
                    drive_actuators(receivedPacket.buffer[0]);
                    break;
                case(CMD_PING_ON):
                    LED_PING_ON;
                    break;
                case(CMD_PING_OFF):
                    LED_PING_OFF;
                    break;
                //case(CMD_PING):
                    //display_ping();
                    //break;
            }
        }
    }
}
