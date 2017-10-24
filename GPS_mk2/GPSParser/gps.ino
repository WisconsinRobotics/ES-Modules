#include <Wire.h>


struct GPS_data {
    uint32_t latitude;
    uint32_t longitude;
    uint32_t speed;
    uint32_t heading;
};


GPS_data GPSparse (char *message, GPS_data parsedData) {
    char messageType[7];
    messageType[6] = '\0';  //needs to be there for strcmp

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
    return 0;
}


setup(){
    Serial.begin(19200);
    Wire.begin(0x2A);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
}

loop(){
    delay(100);
}

void requestEvent(){
    
}

void receiveEvent(int num_bytes){
// function that executes whenever data is received from master
// this function is registered as an event, see setup()
  while (1 < Wire.available()) { // loop through all but the last
    int x = Wire.read(); // receive byte as a character
    Serial.print(x);         // print the character
  }
}
