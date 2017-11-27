#ifndef NMEA_H
#define NMEA_H
#include <avr/io.h>
#include "uart.h"
#include <stdlib.h>

typedef struct GPS_data {
    uint32_t latitude;
    uint32_t longitude;
    uint32_t speed;
    uint32_t heading;
} GPS_data;

void receiveRMCMessage(char* NMEABuffer);
GPS_data GPSparse(char* message, GPS_data parsedData);

#endif
