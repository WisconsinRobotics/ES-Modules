//This code has been modified from Atmel's original to use a circular queue to
//store the data. This should allow for more natural data reception at the cost
//of some speed. - Steven Brandt

#include <avr/io.h> 
#include <avr/interrupt.h>
#include "i2c_packet.h"
#include <util/delay.h>
#include <avr/wdt.h>
// #include <led.h>

// Function Pototype
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

// Function Implementation
void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();
    return;
}

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
static void queue_enQueue(uint8_t data)
{
    byteQueue.dataArray[byteQueue.head] = data;
    byteQueue.head++;
    if(byteQueue.head == BYTE_ARRAY_SIZE)
    {
        byteQueue.head = 0;
    }

    if(byteQueue.numData < BYTE_ARRAY_SIZE)
    {
        byteQueue.numData++;
    }
    else
    {
        byteQueue.overflowFlag = 1;
        byteQueue.tail++;
        if(byteQueue.tail == BYTE_ARRAY_SIZE)
        {
            byteQueue.tail = 0;
        }
    }
}

//Returns data at tail, increments tail
static uint8_t byteQueue_deQueue()
{
    if(byteQueue.numData == 0)
    {
        return 0;
    }
    byteQueue.numData--;

    uint8_t dataToReturn = byteQueue.dataArray[byteQueue.tail];
    byteQueue.tail++;
    if(byteQueue.tail == BYTE_ARRAY_SIZE)
    {
        byteQueue.tail = 0;
    }

    return dataToReturn;
}

static uint8_t i2c_getByte()
{
    return byteQueue_deQueue();
}

//returns how many items are in the queue
static volatile uint8_t i2c_hasData()
{
    return byteQueue.numData;
}

static void byteQueue_flushQueue()
{
    //Re-Initialize byteQueueVariables
    byteQueue.head = 0;
    byteQueue.tail = 0;
    byteQueue.numData = 0;
    byteQueue.overflowFlag = 0;
}

static void packetQueue_flushQueue()
{
    packetQueue.head = 0;
    packetQueue.tail = 0;
    packetQueue.numData = 0;
    packetQueue.overflowFlag = 0;
}

static uint8_t i2c_GetByteWithTimeout(uint8_t *destination, uint8_t timeout_ms)
{
    while (timeout_ms != 0)
    {
        if(byteQueue.numData)
        {
            *destination = byteQueue_deQueue();
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

void i2c_init(uint8_t address)
{
    //Initialize queueVariables
    byteQueue_flushQueue();
    packetQueue_flushQueue();

    //Initialize timeout timer
//  TIMSK2 |= (1<<TOIE2);   //Interrupt on overflow

    //Initialize queue module
    address_g = address;
    TWAR = address<<1;          // Set own queue slave address. Accept queue General Calls.
    TWCR =  (1<<TWEN)|                   // Enable queue-interface and release queue pins.
            (0<<TWIE)|(0<<TWINT)|               // Disable queue Interrupt.
            (0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Do not ACK on any requests, yet.
            (0<<TWWC);                          //

    TWCR =  (1<<TWEN)|                              // queue Interface enabled.
            (1<<TWIE)|(1<<TWINT)|                   // Enable queue Interrupt and clear the flag.
            (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|        // Prepare to ACK next time the Slave is addressed.
            (0<<TWWC);                              //
}

void i2c_checkForPackets()
{
    i2c_checkForBusHogging();
    //uint8_t pathTrace = 0;
    //uint8_t timedOut = 0;
    if(i2c_hasData())
    {
        //pathTrace++;
        uint8_t success = 0;
        uint8_t receiveArray[RECEIVE_ARRAY_SIZE];
        for(int i = 0; i < RECEIVE_ARRAY_SIZE; i++)
        {
            receiveArray[i] = 0x00;
        }
        while(i2c_hasData() && (receiveArray[0] != START) )
        {
            receiveArray[0] = i2c_getByte();
        }
        if(receiveArray[0] == START)
        {
            //pathTrace++;
            uint8_t length;
            if(i2c_GetByteWithTimeout(&length, 10))
            {
                //pathTrace++;
                receiveArray[1] = length;
                if(length <= ( RECEIVE_ARRAY_SIZE-2 ) )
                {
                    //pathTrace++;
                    int i;
                    for(i = 2; i < length+2; i++)
                    {
                        if(!i2c_GetByteWithTimeout(&receiveArray[i], 10))
                        {
                            //Timed out!
                            //timedOut = 1;
                            break;
                        }
                    }
                    if(i == length+2)
                    {
                        //pathTrace++;
                        if(receiveArray[length+1] == STOP)
                        {
                            //pathTrace++;
                            //Calculate parity
                            uint8_t parityByte = 0;
                            for(int j = 1; j < length; j++)
                            {
                                parityByte ^= receiveArray[j];
                            }

                            if(parityByte == receiveArray[length])
                            {
                                //Packet is good!
                                success = 1;
                                packetQueue.packetArray[packetQueue.head].cmd = receiveArray[2];

                                for(int i = 3; i < length; i++)
                                {
                                    packetQueue.packetArray[packetQueue.head].buffer[i-3] = receiveArray[i];
                                }

                                packetQueue.head++;
                                if(packetQueue.head == PACKET_ARRAY_SIZE)
                                {
                                    packetQueue.head = 0;
                                }

                                if(packetQueue.numData < PACKET_ARRAY_SIZE)
                                {
                                    packetQueue.numData++;
                                }
                                else
                                {
                                    packetQueue.overflowFlag = 1;
                                    packetQueue.tail++;
                                    if(packetQueue.tail == PACKET_ARRAY_SIZE)
                                    {
                                        packetQueue.tail = 0;
                                    }
                                }


                            }
                            //else the parity doesn't match!
                        }
                        //else the array doesn't end with a stop byte!
                    }
                    //else one of the bytes timed out!
                }
                //The length byte was too long!
            }
            //else the length byte timed out!
        }
        //else the first byte was NOT the start Byte!
        if(!success)
        {
            static uint8_t numFails = 0;
            numFails++;
            //led_dataOut(numFails);
            
            byteQueue_flushQueue();
            //led_dataOut(1);
            //led_dataOut(pathTrace);
            /*
            for(int i = 0; i < 10; i++)
            {
                led_display(0);
                _delay_ms(50);
                led_display(0xFF);
                _delay_ms(50);
            }
            led_display(pathTrace);
            _delay_ms(4000);
            led_display(timedOut);
            _delay_ms(4000);

            for(int i = 0; i < RECEIVE_ARRAY_SIZE; i++)
            {
                led_display(receiveArray[i]);
                _delay_ms(3000);
                led_display(0xFF);
                _delay_ms(50);
                led_display(0);
                _delay_ms(50);
                led_display(0xFF);
                _delay_ms(50);
                led_display(0);
            }
            //byteQueue_flushQueue();
            //packetQueue_flushQueue();
            */
        }
    }
}

uint8_t i2c_hasPacket()
{
    return packetQueue.numData;
}

uint8_t i2c_getPacket(struct packet *packetDestination)
{
    if(packetQueue.numData == 0)
    {
        return 0;
    }

    *packetDestination = packetQueue.packetArray[packetQueue.tail];

    packetQueue.tail++;
    if(packetQueue.tail == PACKET_ARRAY_SIZE)
    {
        packetQueue.tail = 0;
    }
    packetQueue.numData--;
    return 1;
}

void i2c_setReturnPacket(const struct packet * const packetPointer, uint8_t numDataBytes)
{
    returnData[0] = START;
    returnData[1] = numDataBytes+3;
    returnData[2] = packetPointer->cmd;
    uint8_t i;
    for(i = 3; i < numDataBytes+3; i++)
    {
        returnData[i] = packetPointer->buffer[i-3];
    }

    uint8_t parityByte = 0;
    parityByte ^= returnData[1];
    parityByte ^= returnData[2];
    for(i = 3; i < numDataBytes+3; i++)
    {
        parityByte ^= returnData[i];
    }

    returnData[i] = parityByte;
    returnData[i+1] = STOP;
}

void i2c_checkForBusHogging()
{
    uint16_t timeout = 1000;
    //SDA is on PIN C4
    while(  (timeout != 0) && !(PINC & (1<<PINC4))  )
    {
        timeout--;
        _delay_us(1);
    }

    if(timeout == 0)
    {
        //We better reset the I2C just to be sure
        TWCR = (1<<TWSTO);
        _delay_us(10);

        //Initialize queue module
        TWAR = address_g<<1;                // Set own queue slave address

        TWCR =  (1<<TWEN)|                      // queue Interface enabled.
        (1<<TWIE)|(1<<TWINT)|                   // Enable queue Interrupt and clear the flag.
        (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|        // Prepare to ACK next time the Slave is addressed.
        (0<<TWWC);

        wdt_enable(WDTO_15MS);
        while(1) {};
    }
}

ISR(TIMER2_OVF_vect)
{
    //TWCR &= ~(1<<TWEN);//Disable TWI
    TWCR = (1<<TWSTO);
    _delay_us(10);

    //Initialize queue module
    TWAR = address_g<<1;            // Set own queue slave address. Accept queue General Calls.
    TWCR =  (1<<TWEN)|                   // Enable queue-interface and release queue pins.
    (0<<TWIE)|(0<<TWINT)|               // Disable queue Interrupt.
    (0<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Do not ACK on any requests, yet.
    (0<<TWWC);                          //

    TWCR =  (1<<TWEN)|                              // queue Interface enabled.
    (1<<TWIE)|(1<<TWINT)|                   // Enable queue Interrupt and clear the flag.
    (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|        // Prepare to ACK next time the Slave is addressed.
    (0<<TWWC);
    //Out timer overflowed! shit, that means we gotta reset our I2C
    i2c_checkForBusHogging();
//  led_dataOut(0x1F);  //Debug, show off that this has happened
    TCCR2B &= ~(1<<CS21);   //Disable timeout timer
}




// ********** Interrupt Handlers ********** //
/****************************************************************************
This function is the Interrupt Service Routine (ISR), and called when the queue interrupt is triggered;
that is whenever a queue event has occurred. This function should not be called directly from the main
application.
****************************************************************************/
ISR( TWI_vect )
{
    //reset timeout timer
    //TCNT2 = 0;
    //TCCR2B |= (1<<CS21);  //Prescaler 8. For 2MHz clock, this will take 1ms to overflow

  switch (TWSR)
  {
    case TWIQUEUE_STX_ADR_ACK:                  // Own SLA+R has been received; ACK has been returned
         slaveDataToReturnCurrent = returnData;
    case TWIQUEUE_STX_DATA_ACK:                 // Data byte in TWDR has been transmitted; ACK has been received
         TWDR = *slaveDataToReturnCurrent;
         slaveDataToReturnCurrent++;

         TWCR = (1<<TWEN)|                      // queue Interface enabled
             (1<<TWIE)|(1<<TWINT)|              // Enable queue Interupt and clear the flag to send byte
             (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|   // 
             (0<<TWWC);                         //
         break;

    case TWIQUEUE_STX_DATA_NACK:                // Data byte in TWDR has been transmitted; NACK has been received. 
                                                // I.e. this could be the end of the transmission.                                
         TWCR = (1<<TWEN)|                          // Enable queue-interface and release queue pins
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
         TWCR = (1<<TWEN)|                          // queue Interface enabled
                (1<<TWIE)|(1<<TWINT)|               // Enable TWI Interupt and clear the flag to send byte
                (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Expect ACK on this transmission
                (0<<TWWC);  
         break;

    case TWIQUEUE_SRX_ADR_DATA_ACK:             // Previously addressed with own SLA+W; data has been received; ACK has been returned
    case TWIQUEUE_SRX_GEN_DATA_ACK:             // Previously addressed with general call; data has been received; ACK has been returned
         queue_enQueue(TWDR);      
                                                    // Reset the queue Interrupt to wait for a new event.
         TWCR = (1<<TWEN)|                          // queue Interface enabled
                (1<<TWIE)|(1<<TWINT)|               // Enable queue Interrupt and clear the flag to send byte
                (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Send ACK after next reception
                (0<<TWWC);                          // 
         break;

    case TWIQUEUE_SRX_STOP_RESTART:             // A STOP condition or repeated START condition has been received while still addressed as Slave    
                                                // Enter not addressed mode and listen to address match
         TWCR = (1<<TWEN)|                          // Enable queue-interface and release queue pins
                (1<<TWIE)|(1<<TWINT)|   // Enable interrupt and clear the flag
                (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Wait for new address match
                (0<<TWWC);                          //
      
         break;           

    case TWIQUEUE_SRX_ADR_DATA_NACK:            // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
    case TWIQUEUE_SRX_GEN_DATA_NACK:            // Previously addressed with general call; data has been received; NOT ACK has been returned
    case TWIQUEUE_STX_DATA_ACK_LAST_BYTE:       // Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received
//  case TWIQUEUE_NO_STATE                      // No relevant state information available; queueNT = “0”
    case TWIQUEUE_BUS_ERROR:                    // Bus error due to an illegal START or STOP condition
         TWCR =   (1<<TWSTO)|(1<<TWINT);        //Recover from queue_BUS_ERROR, this will release the SDA and SCL pins thus enabling other devices to use the bus
         break;

    default:     
         TWCR = (1<<TWEN)|                          // Enable queue-interface and release queue pins
                (1<<TWIE)|(1<<TWINT)|               // Keep interrupt enabled and clear the flag
                (1<<TWEA)|(0<<TWSTA)|(0<<TWSTO)|    // Acknowledge on any new requests.
                (0<<TWWC);                          //
  }
}
