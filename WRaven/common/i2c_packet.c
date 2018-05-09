//This code has been modified from Atmel's original to use a circular queue to
//store the data. This should allow for more natural data reception at the cost
//of some speed. - Steven Brandt

#include <avr/io.h> 
#include <avr/interrupt.h>
#include "i2c_packet.h"
#include <util/delay.h>
#include <util/atomic.h>

volatile uint8_t i2c_has_data_flag = 0;

struct queueData
{
    uint8_t volatile head;
    uint8_t volatile tail;
    uint8_t volatile numData;
    uint8_t volatile dataArray[BYTE_ARRAY_SIZE];
    uint8_t volatile overflowFlag;
};

struct packetQueueData
{
    uint8_t head;
    uint8_t tail;
    uint8_t numData;
    struct packet packetArray[PACKET_ARRAY_SIZE];
    uint8_t overflowFlag;
};

static volatile struct queueData byteQueue;
static struct packetQueueData packetQueue;
static uint8_t returnData[20];
static uint8_t *slaveDataToReturnCurrent;
static uint8_t address_g = 1;

/********************
 * Static functions
 ********************/

//Places data at head, increments head
static void queue_enQueue(uint8_t data) {
    //ATOMIC_BLOCK(ATOMIC_FORCEON) {
	    byteQueue.dataArray[byteQueue.head] = data;
        byteQueue.head++;
        if(byteQueue.head == BYTE_ARRAY_SIZE)
            byteQueue.head = 0;

        if(byteQueue.numData < BYTE_ARRAY_SIZE)
            byteQueue.numData++;
        else {
            byteQueue.overflowFlag = 1;
            byteQueue.tail++;
            if(byteQueue.tail == BYTE_ARRAY_SIZE)
                byteQueue.tail = 0;
        }
    //}
}

//Returns data at tail, increments tail
static uint8_t byteQueue_deQueue() {
	cli();
	if(byteQueue.numData == 0){
		sei();
		return 0;
	}
    byteQueue.numData--;

    uint8_t dataToReturn = byteQueue.dataArray[byteQueue.tail];
    byteQueue.tail++;
    if(byteQueue.tail == BYTE_ARRAY_SIZE)
        byteQueue.tail = 0;
	sei();
    return dataToReturn;
}

static uint8_t i2c_getByte() {
	cli();
	uint8_t data = byteQueue_deQueue();
    sei();
	return data;
}

//returns how many items are in the queue
static volatile uint8_t i2c_hasData() {
	cli();
	uint8_t data = byteQueue.numData;
	sei();
	return data;
}

static void byteQueue_flushQueue() {
    //Re-Initialize byteQueueVariables
	byteQueue.head = 0;
    byteQueue.tail = 0;
    byteQueue.numData = 0;
    byteQueue.overflowFlag = 0;
}

static void packetQueue_flushQueue() {
    packetQueue.head = 0;
    packetQueue.tail = 0;
    packetQueue.numData = 0;
    packetQueue.overflowFlag = 0;
}

static uint8_t i2c_GetByteWithTimeout(uint8_t *destination, uint8_t timeout_ms) {
	while (timeout_ms != 0) {
        if(byteQueue.numData) {    //if there is data in the packet?
            *destination = byteQueue_deQueue();	//grabs the next btye after the start btye. this is the length
		    return 1;
        }
        timeout_ms--;
        _delay_ms(1);
    }
    return 0;
}


/************************
 * Non-static functions
 ************************/

void i2c_init(uint8_t address) {
    //Initialize queueVariables
    byteQueue_flushQueue();
    packetQueue_flushQueue();

    //Initialize queue module
    address_g = address;
    TWAR0 = address<<1;          // Set own queue slave address. Accept queue General Calls.
    TWCR0 =  (1<<TWEN)|                   // Enable queue-interface and release queue pins.
            (0<<TWIE)|(0<<TWINT)|               // Disable queue Interrupt.
            (0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Do not ACK on any requests, yet.
            (0<<TWWC);                          //

    TWCR0 = (1<<TWEN)|                              // queue Interface enabled.
            (1<<TWIE)|(1<<TWINT)|                   // Enable queue Interrupt and clear the flag.
            (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|        // Prepare to ACK next time the Slave is addressed.
            (0<<TWWC);                              //
}

void i2c_checkForPackets() {
    if(i2c_hasData()) {
        uint8_t success = 0;
        uint8_t receiveArray[RECEIVE_ARRAY_SIZE];
        for(int i = 0; i < RECEIVE_ARRAY_SIZE; i++)	//sets recive array to all 0s
            receiveArray[i] = 0x00;

        while(i2c_hasData() && (receiveArray[0] != START) ) //waits if start of packet is not AB
            receiveArray[0] = i2c_getByte();

        if(receiveArray[0] == START) {	//stop waiting
            uint8_t length;
            if(i2c_GetByteWithTimeout(&length, 10)) {	//if it gets the length of the packet in 10 milliseconds
                receiveArray[1] = length;
                if(length <= ( RECEIVE_ARRAY_SIZE-2 ) ) { //takes into account parity and stop bytes in length
                    int i;
                    for(i = 2; i < length+2; i++) {
                        if(!i2c_GetByteWithTimeout(&receiveArray[i], 10))	//reads the rest of the packet
                            break;
                    }
                    if(i == length+2) { //if it has read through the packet
                        if(receiveArray[length+1] == STOP) {
                            //Calculate parity
                            uint8_t parityByte = 0;
                            for(int j = 1; j < length; j++)
                                parityByte ^= receiveArray[j];

                            if(parityByte == receiveArray[length]) {
                                //Packet is good!
                                success = 1;
                                packetQueue.packetArray[packetQueue.head].cmd = receiveArray[2];	//gets the command fro the packet

                                for(int i = 3; i < length; i++)	//stores all of the parameters
                                    packetQueue.packetArray[packetQueue.head].buffer[i-3] = receiveArray[i];

                                packetQueue.head++;
                                if(packetQueue.head == PACKET_ARRAY_SIZE)
                                    packetQueue.head = 0;

                                if(packetQueue.numData < PACKET_ARRAY_SIZE)	//we have another packet
                                    packetQueue.numData++;
                                else {    //we have too many packets
                                    packetQueue.overflowFlag = 1;
                                    packetQueue.tail++;
                                    if(packetQueue.tail == PACKET_ARRAY_SIZE)
                                        packetQueue.tail = 0;
                                }
                            } //else the parity doesn't match!
                        } //else the array doesn't end with a stop byte!
                    } //else one of the bytes timed out!
                } //The length byte was too long!
            } //else the length byte timed out!
        } //else the first byte was NOT the start Byte!
        if(!success) {
            static uint8_t numFails = 0;
            numFails++;
        }
    }
}

uint8_t i2c_hasPacket() {
    return packetQueue.numData;
}

uint8_t i2c_getPacket(struct packet *packetDestination) {
    if(packetQueue.numData == 0)
        return 0;

    *packetDestination = packetQueue.packetArray[packetQueue.tail];

    packetQueue.tail++;
    if(packetQueue.tail == PACKET_ARRAY_SIZE)
        packetQueue.tail = 0;

    packetQueue.numData--;
    return 1;
}

void i2c_setReturnPacket(const struct packet * const packetPointer, uint8_t numDataBytes) {
    //ATOMIC_BLOCK(ATOMIC_FORCEON){
        returnData[0] = START;
        returnData[1] = numDataBytes+3;
        returnData[2] = packetPointer->cmd;
        uint8_t i;
        for(i = 3; i < numDataBytes+3; i++)
            returnData[i] = packetPointer->buffer[i-3];

        uint8_t parityByte = 0;
        parityByte ^= returnData[1];
        parityByte ^= returnData[2];
        for(i = 3; i < numDataBytes+3; i++)
            parityByte ^= returnData[i];

        returnData[i] = parityByte;
        returnData[i+1] = STOP;
    //}
}

ISR(TIMER2_OVF_vect) {
    TWCR0 = (1<<TWSTO);
    _delay_us(10);

    //Initialize queue module
    TWAR0 = address_g<<1;               // Set own queue slave address. Accept queue General Calls.
    TWCR0 =  (1<<TWEN)|                 // Enable queue-interface and release queue pins.
    (0<<TWIE)|(0<<TWINT)|               // Disable queue Interrupt.
    (0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Do not ACK on any requests, yet.
    (0<<TWWC);                          //

    TWCR0 =  (1<<TWEN)|                 // queue Interface enabled.
    (1<<TWIE)|(1<<TWINT)|               // Enable queue Interrupt and clear the flag.
    (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Prepare to ACK next time the Slave is addressed.
    (0<<TWWC);
    //Out timer overflowed! shit, that means we gotta reset our I2C
    TCCR2B &= ~(1<<CS21);   //Disable timeout timer
}

// ********** Interrupt Handlers ********** //
/****************************************************************************
This function is the Interrupt Service Routine (ISR), and called when the queue interrupt is triggered;
that is whenever a queue event has occurred. This function should not be called directly from the main
application.
****************************************************************************/
ISR( TWI0_vect ) {
    switch (TWSR0) {
        case TWIQUEUE_STX_ADR_ACK:                  // Own SLA+R has been received; ACK has been returned
            slaveDataToReturnCurrent = returnData;
        case TWIQUEUE_STX_DATA_ACK:                 // Data byte in TWDR has been transmitted; ACK has been received
            TWDR0 = *slaveDataToReturnCurrent;
            slaveDataToReturnCurrent++;

            TWCR0 = (1<<TWEN)|                      // queue Interface enabled
                (1<<TWIE)|(1<<TWINT)|              // Enable queue Interupt and clear the flag to send byte
                (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|   // 
                (0<<TWWC);                         //
            break;

        case TWIQUEUE_STX_DATA_NACK:                // Data byte in TWDR has been transmitted; NACK has been received. 
                                                    // I.e. this could be the end of the transmission.                                
            TWCR0 = (1<<TWEN)|                          // Enable queue-interface and release queue pins
                    (1<<TWIE)|(1<<TWINT)|               // Keep interrupt enabled and clear the flag
                    (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Answer on next address match
                    (0<<TWWC);                          //
            break;     

        case TWIQUEUE_SRX_GEN_ACK:                  // General call address has been received; ACK has been returned
        case TWIQUEUE_SRX_GEN_ACK_M_ARB_LOST:       // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
        case TWIQUEUE_SRX_ADR_ACK:                  // Own SLA+W has been received ACK has been returned
        case TWIQUEUE_SRX_ADR_ACK_M_ARB_LOST:       // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned    
                                                    // Dont need to clear queue_S_statusRegister.generalAddressCall due to that it is the default state.
                                                    // Reset the queue Interupt to wait for a new event.
            TWCR0 = (1<<TWEN)|                          // queue Interface enabled
                    (1<<TWIE)|(1<<TWINT)|               // Enable TWI Interupt and clear the flag to send byte
                    (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Expect ACK on this transmission
                    (0<<TWWC);  
            break;

        case TWIQUEUE_SRX_ADR_DATA_ACK:             // Previously addressed with own SLA+W; data has been received; ACK has been returned
        case TWIQUEUE_SRX_GEN_DATA_ACK:             // Previously addressed with general call; data has been received; ACK has been returned
            //i2c_has_data_flag = 1;
			queue_enQueue(TWDR0);      
                                                        // Reset the queue Interrupt to wait for a new event.
            TWCR0 = (1<<TWEN)|                          // queue Interface enabled
                    (1<<TWIE)|(1<<TWINT)|               // Enable queue Interrupt and clear the flag to send byte
                    (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Send ACK after next reception
                    (0<<TWWC);                          // 
            break;

        case TWIQUEUE_SRX_STOP_RESTART:             // A STOP condition or repeated START condition has been received while still addressed as Slave    
                                                    // Enter not addressed mode and listen to address match
            TWCR0 = (1<<TWEN)|                          // Enable queue-interface and release queue pins
                    (1<<TWIE)|(1<<TWINT)|   // Enable interrupt and clear the flag
                    (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Wait for new address match
                    (0<<TWWC);                          //
        
            break;           

        case TWIQUEUE_SRX_ADR_DATA_NACK:            // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
        case TWIQUEUE_SRX_GEN_DATA_NACK:            // Previously addressed with general call; data has been received; NOT ACK has been returned
        case TWIQUEUE_STX_DATA_ACK_LAST_BYTE:       // Last data byte in TWDR has been transmitted (TWEA = �0�); ACK has been received
    //  case TWIQUEUE_NO_STATE                      // No relevant state information available; queueNT = �0�
        case TWIQUEUE_BUS_ERROR:                    // Bus error due to an illegal START or STOP condition
            TWCR0 =   (1<<TWSTO)|(1<<TWINT);        //Recover from queue_BUS_ERROR, this will release the SDA and SCL pins thus enabling other devices to use the bus
            break;

        default:     
            TWCR0 = (1<<TWEN)|                          // Enable queue-interface and release queue pins
                    (1<<TWIE)|(1<<TWINT)|               // Keep interrupt enabled and clear the flag
                    (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Acknowledge on any new requests.
                    (0<<TWWC);                          //
    }
}
