#ifndef USART_H
#define USART_H
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#define F_CPU 8000000UL
#define RING_BUFFER_SIZE 64
#define USART_BAUD 19200
//#define UBRR 26542
#define BAUD_PRESCALE (((F_CPU / (USART_BAUD * 16UL))) - 1)
#define UBRR BAUD_PRESCALE

//extern "C" void USART_TX_vect(void) __attribute__ ((signal));
extern "C" void USART_RX_vect(void) __attribute__ ((signal));

typedef struct {
    uint32_t producer;
    uint32_t consumer;
    uint16_t buffer_size;
    volatile char array[64];
} PC_Buffer;

void bufferInit(PC_Buffer *buffer, uint16_t buffer_size);
void usart_init(void);
void bufferAdd(PC_Buffer *buffer, char data);
bool bufferEmpty(PC_Buffer *buffer);
bool bufferFull(PC_Buffer *buffer);
void bufferRemove(PC_Buffer *buffer, char *data);
void sendByte(char data);
char getByte();
void USART_RX_vect(void);
#endif
