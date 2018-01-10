/*
 * GPS.c
 *
 * Created: 10/12/2016 12:40:39 AM
 * Author : Dakota
 */

#include "main.h"

int main(void) {

    USART0Init();
     
    //Initialize I2C
    i2c_init(ADDRESS);
    sei();
    
    struct packet receivedPacket;
    struct packet returnPacket;

    //Declare struct for parsed data from sensor 
    GPS_data parsedData = {0, 0, 0, 0};
    
    //specify return packet
    i2c_setReturnPacket(&returnPacket, 4);
    while (1) {

        //Wait to receive a valid RMC message
        //receiveRMCMessage(&NMEABuffer);
        parsedData = getGPS();

        //Set return data for master
        i2c_setReturnPacket(&returnPacket, 4);

        //Check for requests from master
        i2c_checkForPackets();
        
        if (i2c_hasPacket()) {
            //toggle cmd LED 
            PORTB |= (1 << PORTB0);
            cmd_counter = 5;
             
            i2c_getPacket(&receivedPacket);

            //set buffer values depending on the requested data
            switch (receivedPacket.cmd) {
                returnPacket.cmd = receivedPacket.cmd;
                
                case PING_ON:
					PORTB |= (1<<PORTB1);
                    break;

                case PING_OFF:
					PORTB &= ~(1<<PORTB1);
                    break; 

                case GET_LAT:
                    i2c_setReturnData(&returnPacket, parsedData.latitude);
                    break;

                case GET_LONG:
                    i2c_setReturnData(&returnPacket, parsedData.longitude);
                    break;

                case GET_SPEED:
                    i2c_setReturnData(&returnPacket, parsedData.speed);
                    break;

                case GET_HEADING:
                    i2c_setReturnData(&returnPacket, parsedData.heading);
                    break;

                default:
                    i2c_setReturnData(&returnPacket, 0);
                    break;
            }
        }

        if (cmd_counter != 0 ) {
            cmd_counter--;
            if (cmd_counter == 0)
                PORTB &= ~(1 << PORTB0); 
        }
        
        if (alive_counter == 10) {
            PORTB ^= 1 << PORTB2;
            alive_counter = 0;
        }
       
        alive_counter++;
    }
}
