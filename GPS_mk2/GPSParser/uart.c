#include "uart.h"
#include <avr/io.h>

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

