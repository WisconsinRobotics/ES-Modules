#include "nmea.h"
#include "uart.h"
#include <string.h>

GPS_data GPSparse (char *message) {
    char messageType[7];
    messageType[6] = '\0';  //needs to be there for strcmp
    GPS_data parsedData;
    memcpy(messageType, message, 6 * sizeof(char));

    //Looking only for GPRMC type messages.
    if (strcmp(messageType, "$GPRMC") == 0 ) {
        //get pointers to latitude and longitude
        uint16_t count = 0;
        uint16_t i = 0;
        
        //declare pointers in string for desired variables
        char* latptr = &message[0];
        char* lonptr = &message[0];
        char* heading_ptr = &message[0];
        char* speed_ptr = &message[0];
       

        //count commas to get to the correct value, and store pointer to it 
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

        //Tokenize and convert to integer
        char* token;

        token = strtok(latptr + 1, ",");
        for (int i = 4; i < 9; i++)
            token[i] = token[i+1]; 
        token[9] = '.';
        parsedData.latitude = atol(token);

        token = strtok(lonptr + 1, ",");
        for (int i = 5; i < 10; i++)
            token[i] = token[i+1]; 
        token[11] = '.';
        parsedData.longitude = atol(token);

        token = strtok(speed_ptr + 1, ",");
        for (int i = 3; i < 5; i++)
            token[i] = token[i+1]; 
        token[5] = '.';
        parsedData.speed = atol(token);

        token = strtok(heading_ptr + 1, ",");
        for (int i = 5; i < 4; i++)
            token[i] = token[i+1]; 
        token[4] = '.';
        parsedData.heading = atol(token);
        
        return parsedData;

    }
    return parsedData;
}

void receiveRMCMessage(char* NMEABuffer){
    uint8_t valid_rmc = 0;
    char recvByte;
    while (!valid_rmc) {
        
        //receive serial byte from sensor 
        recvByte = USART0ReceiveByte();
        
        //Dump characters until a start byte is found
        while (recvByte != '$') 
            recvByte = USART0ReceiveByte();
        
        //Read bytes from sensor until reaching the end of string
        for (int i = 0; i < 100; i++) {
            NMEABuffer[i] = recvByte;
            if (recvByte == '\n')  
                break;
            recvByte = USART0ReceiveByte();
        }
        
        //The first characters of the string are the message type
        char messageType[7];
        messageType[6] = '\0';  //c strings need this 
        memcpy(messageType, NMEABuffer, 6 * sizeof(char));

        //If the message type is GPRMC, message is valid, exit while loop
        if (strcmp(messageType, "$GPRMC") == 0 ) 
            valid_rmc = 1; 
    }

}

GPS_data getGPS(){
    char NMEABuffer[100] = "$GPRMC,020106.2,V,0000.00000,N,00000.00000,W,000.18,300.6,150916,001.8,W,A*3D";
    receiveRMCMessage(NMEABuffer);
    GPS_data parsedData = GPSparse(NMEABuffer);
    return parsedData;
}
