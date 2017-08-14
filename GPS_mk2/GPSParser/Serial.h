void Serial::begin(void) {
    UCSR0C |= (1 << UCSZ00) | (1 << UCSZ01);    //Set For 8 bit character
    UBRR0L = 25;                                //Value set from tech sheet
    UCSR0B |= (1 << RXEN0)  | (1 << TXEN0);     //Enable Transmit and Receive
}

void Serial::write(char u8Data) {
    //wait while previous byte is completed
    while (!(UCSR0A & (1 << UDRE0))) {};

    // Transmit data
    UDR0 = u8Data;
}

char Serial::read() {
    // Wait for byte to be received
    while (!(UCSR0A & ( 1 << RXC0 ))) {}

    // Return received data
    return UDR0;
}


