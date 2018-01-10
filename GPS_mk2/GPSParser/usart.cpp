#include "usart.h"
#include <string.h>

volatile bool rx_flag = false;
PC_Buffer rx_buffer;

void usart_init() {
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01); //set 8 bit frame and 1 stop bit
    UBRR0H = (uint8_t) UBRR>>8; //set baud
    UBRR0L = (uint8_t) UBRR;
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1 << RXC0); //enable TX and RX

    //initialize the ring buffers
    bufferInit(&rx_buffer, RING_BUFFER_SIZE);
}


void bufferInit(PC_Buffer *buffer, uint16_t buffer_size){
    buffer->producer = 0;
    buffer->consumer = 0;
    buffer->buffer_size = 64; //until I sort out dynamic allocation
}

bool bufferEmpty(PC_Buffer *buffer){
    if (buffer->producer == buffer->consumer)
        return true;
    else
        return false;
}

bool bufferFull(PC_Buffer *buffer){
    if(buffer->producer - buffer->consumer == buffer->buffer_size)
        return true;
    else
        return false;
}

void bufferRemove(PC_Buffer *buffer, char *data){
    *data = buffer->array[buffer->consumer % buffer->buffer_size];
    buffer->consumer++;
}

void bufferAdd(PC_Buffer *buffer, char data){
    buffer->array[buffer->producer % buffer->buffer_size] = data;
    buffer->producer++;
}

//allows me to access members of usart
USART0* USART0::pUsart;

ISR(USART_RX_vect){
    char data = UDR0;
    USART0::pUsart->bufferAdd(&rx_buffer, data);
    rx_flag = true;
}

void sendByte(char data)
{
   /* Wait for empty transmit buffer */
   while (!( UCSR0A & (1<< UDRE0 )));

   /* Put data into buffer, sends the data */
   UDR0 = data;
}

char getByte() {
    if (!bufferEmpty(&rx_buffer)){
        char data;
        bufferRemove(&rx_buffer, &data);
        return data;
    }
    else
        return -1;
}