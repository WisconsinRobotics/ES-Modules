
#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <avr/wdt.h>
#include "i2c_packet.h"
#include "nmea.h"
#include "uart.h"

//define UART paramters
#define USART_BAUDRATE 19200
#define UBRR_VALUE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

//define i2c address
#define ADDRESS 42

//define commands for i2c requests
#define PING_ON 0x01
#define PING_OFF 0x02
#define SOFT_RST 0x03
#define GET_LAT 0x04
#define GET_LONG 0x05
#define GET_SPEED 0x06
#define GET_HEADING 0x07


//various counters
uint8_t alive_counter = 0;
uint8_t cmd_counter = 0;
