#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <avr/interrupt.h>

void USART0Init(void);
void USART0SendByte(char u8Data);
char USART0ReceiveByte();

#endif