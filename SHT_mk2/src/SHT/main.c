/*	
 * main.c
 *
 * Created: 11/2/2016 8:11:54 PM
 * Author : Death
 */ 

#define F_CPU 8000000UL
#define PING_ON 0x01
#define PING_OFF 0x02
#define GET_TEMP 0x03
#define GET_HUMID 0x04

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "i2c_packet.h"
#include "SHT1x.h"

int main(void) {
    DDRB = 0b00010011;
    DDRC = 0b00000010;
    DDRD = 0b11000111;
    PORTC &= ~(1<<PORTC1);

    i2c_init(77);
    sei();
    int counter = 0;
    packet receivedPacket;
    packet returnPacket;

    while (1) {
        returnPacket.cmd = GET_TEMP;
      
        uint16_t tempC = readTemperatureC();
        returnPacket.buffer[2] = (uint8_t) (tempC>>8);
        returnPacket.buffer[3] = (uint8_t) (tempC);
            
        uint16_t humidity = readHumidity();
        returnPacket.buffer[0] = (uint8_t) (humidity>>8);
        returnPacket.buffer[1] = (uint8_t) (humidity);


        i2c_setReturnPacket(&returnPacket, 4);
        i2c_checkForPackets();

        if(i2c_hasPacket()) {		

            i2c_getPacket(&receivedPacket);
            switch(receivedPacket.cmd) {	
                case PING_ON:
                    PORTD |= (1<<PORTD1);
                    break;

                case PING_OFF:
                    PORTD &= ~(1<<PORTD1);
                    break;	
      
                case GET_TEMP:
                    break;

                case GET_HUMID:
                    break;
            }
        }

        counter++;
        if(counter>=1){
            counter = 0;
            PORTD ^= (1<<PORTD0);
        }
    }
}
