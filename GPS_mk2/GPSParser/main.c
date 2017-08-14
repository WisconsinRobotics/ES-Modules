/*
 * GPS.c
 *
 * Created: 10/12/2016 12:40:39 AM
 * Author : Dakota
 */
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <avr/wdt.h>
#include "i2c_packet.h"
#define USART_BAUDRATE 19200
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)
#define ADDRESS 42
#define PING_ON 0x01
#define PING_OFF 0x02
#define SOFT_RST 0x03
#define GET_GPS 0x05

struct GPS_data {
    uint32_t latitude;
    uint32_t longitude;
    uint32_t speed;
    uint32_t heading;
};


void USART0Init(void) {
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);    //Set For 8 bit character
    UBRR0L = 25;                                //Value set from tech sheet
    UCSR0B |= (1 << RXEN0)  | (1 << TXEN0);     //Enable Transmit and Receive
}

void USART0SendByte(char u8Data) {
    //wait while previous byte is completed
    while (!(UCSR0A & (1 << UDRE0))) {};

    // Transmit data
    UDR0 = u8Data;
}

char USART0ReceiveByte() {
    // Wait for byte to be received
    while (!(UCSR0A & ( 1 << RXC0 ))) {}

    // Return received data
    return UDR0;
}

void decimalToDMS(float DDec, uint16_t *parsedResponse) {
    float degValue = DDec / 100;
    uint16_t degrees = (uint16_t) degValue;

    float decMinutesSeconds = (degValue - degrees) / 0.60;
    float minuteValue = decMinutesSeconds * 60;
    uint16_t minutes = (uint16_t) minuteValue;

    uint16_t seconds = (uint16_t) ((minuteValue - minutes) * 60 * 10); //multiplies by 10 to avoid precision loss

    *parsedResponse = degrees;
    *(parsedResponse + 1) = minutes;
    *(parsedResponse + 2) = seconds;
}

GPS_data GPSparse (char *message, GPS_data parsedData) {
    char messageType[7];
    messageType[6] = '\0';  //needs to be there for strcmp

    memcpy(messageType, message, 6 * sizeof(char));

    //Looking only for GPRMC type messages.
    if (strcmp(messageType, "$GPRMC") == 0 ) {
        //get pointers to latitude and longitude
        uint16_t count = 0;
        uint16_t i = 0;

        char* latptr = &message[0];
        char* lonptr = &message[0];
        char* heading_ptr = &message[0];
        char* speed_ptr = &message[0];
        
        while (count < 8) {
            if (message[i] == ',' && count < 3) {
                latptr = &message[i];
                count++;
            }
            else if (message[i] == ',' && count >= 3 && count < 5) {
                lonptr = &message[i];
                count++;
            }
            else if (message[i] == ',' && count >= 5 && count < 7) {
                speed_ptr = &message[i];
                count++;
            }
            else if (message[i] == ',' && count >= 7) {
                heading_ptr = &message[i];
                count++;
            }
            i++;
        }

        //Tokenize and convert to float
        char* token;
        token = strtok(latptr + 1, ",");
        if (token == NULL)
            return 0; 
        for (int i = 4; i < 9; i++)
            token[i] = token[i+1]; 
        token[9] = '.';
        parsedData.latitude = atoi(token);

        token = strtok(lonptr + 1, ",");
        if (token == NULL)
            return 0;
        for (int i = 5; i < 10; i++)
            token[i] = token[i+1]; 
        token[11] = '.';
        parsedData.longitude = atoi(token);


        token = strtok(speed_ptr + 1, ",");
        if (token == NULL)
            return 0;
        for (int i = 3; i < 5; i++)
            token[i] = token[i+1]; 
        token[5] = '.';
        parsedData.speed = atoi(token);


        token = strtok(heading_ptr + 1, ",");
        if (token == NULL)
            return 0;
        for (int i = 5; i < 4; i++)
            token[i] = token[i+1]; 
        token[4] = '.';
        parsedData.heading = atol(token);
        
        return parsedData;

    }
    return 0;
}


int main(void) {

    USART0Init();

    //Set LED pins as outputs
    DDRB |= 1 << DDB0;
    DDRB |= 1 << DDB1;
    DDRB |= 1 << DDB2;

    //Initialize I2C
    i2c_init(ADDRESS);
    sei();

    struct packet returnPacket;
    struct packet receivedPacket;

    for (int i = 0; i < 6; i++) {
        returnPacket.buffer[i] = 0;
    }
    returnPacket.cmd = 0x05;

	//  uint8_t wdt_counter = 0;

    char NMEABuffer[100] = "$GPRMC,020106.2,V,0000.00000,N,00000.00000,W,000.18,300.6,150916,001.8,W,A*3D";
    char recvByte = ' ';
    GPS_data parsedData;

    //TWIqueue buffer
    i2c_setReturnPacket(&returnPacket, 15);

    //alive LED counter
    uint8_t alive_counter = 0;
    uint8_t cmd_counter = 0;
    uint8_t valid_rmc = 0;
    while (1) {
        valid_rmc = 0;
        while (!valid_rmc) {
            recvByte = USART0ReceiveByte();

            while (recvByte != '$') {
                recvByte = USART0ReceiveByte();
            }

            for (int i = 0; i < 100; i++) {
                NMEABuffer[i] = recvByte;
                if ( recvByte == '\n') {
                    break;
				}
                recvByte = USART0ReceiveByte();
            }

            char messageType[7];
            messageType[6] = '\0';  //needs to be there for strcmp
            memcpy(messageType, NMEABuffer, 6 * sizeof(char));
            if (strcmp(messageType, "$GPRMC") == 0 ) {
                valid_rmc = 1;
            }
        }


        GPS_data parsedData = GPSparse(NMEABuffer);
        
       
        //Set return data for master
        i2c_setReturnPacket(&returnPacket, 4);

        //Check for requests from master
        i2c_checkForPackets();
        if (i2c_hasPacket()) {
            PORTB |= (1 << PORTB0);
            cmd_counter = 5;
             
            i2c_getPacket(&receivedPacket);
            switch (receivedPacket.cmd) {
            case GET_LAT:
                i2c_setReturnData(&returnPacket, GPS_data.latitude); 
                break;

            case GET_LONG:
                i2c_setReturnData(&returnPacket, GPS_data.longitude); 
                break;

            case GET_SPEED:
                i2c_setReturnData(&returnPacket, GPS_data.speed); 
                break;

            case GET_HEADING:
                i2c_setReturnData(&returnPacket, GPS_data.heading); 
                break;
            }
        }

        //flash the alive LED every 10 cycles
        
        if (cmd_counter != 0 ) {
            cmd_counter--;
            if (cmd_counter == 0) {
                PORTB &= ~(1 << PORTB0);
            }
        }
        if (alive_counter == 1) {
            PORTB ^= 1 << PORTB2;
            alive_counter = 0;
        }
        alive_counter++;

    }
}
