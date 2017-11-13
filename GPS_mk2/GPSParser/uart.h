#ifndef UART_H
#define UART_H

void USART0Init(void);
void USART0SendByte(char u8Data);
char USART0ReceiveByte();

#endif