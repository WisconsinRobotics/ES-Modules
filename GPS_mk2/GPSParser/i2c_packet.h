//This code has been modified from Atmel's original to use a circular queue to
//store the data. This should allow for more natural data reception at the cost
//of some speed. - Steven Brandt
//TODO There's a bunch of legacy crap in here I don't use. Clean it out!

/****************************************************************************
  TWIqueue Status/Control register definitions
****************************************************************************/
#include <stdlib.h>
#define F_CPU 8000000UL

struct packet
{
	uint8_t cmd;
	uint8_t buffer[32];
};

#define START 0xAB
#define STOP 0xCD

#define BYTE_ARRAY_SIZE		64 
#define PACKET_ARRAY_SIZE	16

#define RECEIVE_ARRAY_SIZE 20


/****************************************************************************
  Function definitions
****************************************************************************/
void i2c_init(uint8_t address);
void i2c_checkForPackets();
uint8_t i2c_hasPacket();
uint8_t i2c_getPacket(struct packet *packetDestination);
void i2c_setReturnPacket(const struct packet * const packetPointer, uint8_t numDataBytes);
void i2c_setReturnData(packet * pkt_ptr, uint32_t returnData);
void i2c_checkForBusHogging();

/****************************************************************************
  Bit and byte definitions
****************************************************************************/
#define TWIQUEUE_READ_BIT  0   // Bit position for R/W bit in "address byte".
#define TWIQUEUE_ADR_BITS  1   // Bit position for LSB of the slave address bits in the init byte.
#define TWIQUEUE_GEN_BIT   0   // Bit position for LSB of the general call bit in the init byte.

#define TRUE          1
#define FALSE         0

/****************************************************************************
  TWIQUEUE State codes
****************************************************************************/
// General TWIQUEUE Master status codes                      
#define TWIQUEUE_START                  0x08  // START has been transmitted  
#define TWIQUEUE_REP_START              0x10  // Repeated START has been transmitted
#define TWIQUEUE_ARB_LOST               0x38  // Arbitration lost

// TWIqueue Master Transmitter status codes                      
#define TWIQUEUE_MTX_ADR_ACK            0x18  // SLA+W has been transmitted and ACK received
#define TWIQUEUE_MTX_ADR_NACK           0x20  // SLA+W has been transmitted and NACK received 
#define TWIQUEUE_MTX_DATA_ACK           0x28  // Data byte has been transmitted and ACK received
#define TWIQUEUE_MTX_DATA_NACK          0x30  // Data byte has been transmitted and NACK received 

// TWIqueue Master Receiver status codes  
#define TWIQUEUE_MRX_ADR_ACK            0x40  // SLA+R has been transmitted and ACK received
#define TWIQUEUE_MRX_ADR_NACK           0x48  // SLA+R has been transmitted and NACK received
#define TWIQUEUE_MRX_DATA_ACK           0x50  // Data byte has been received and ACK tramsmitted
#define TWIQUEUE_MRX_DATA_NACK          0x58  // Data byte has been received and NACK tramsmitted

// TWIQUEUE Slave Transmitter status codes
#define TWIQUEUE_STX_ADR_ACK            0xA8  // Own SLA+R has been received; ACK has been returned
#define TWIQUEUE_STX_ADR_ACK_M_ARB_LOST 0xB0  // Arbitration lost in SLA+R/W as Master; own SLA+R has been received; ACK has been returned
#define TWIQUEUE_STX_DATA_ACK           0xB8  // Data byte in TWDR has been transmitted; ACK has been received
#define TWIQUEUE_STX_DATA_NACK          0xC0  // Data byte in TWDR has been transmitted; NOT ACK has been received
#define TWIQUEUE_STX_DATA_ACK_LAST_BYTE 0xC8  // Last data byte in TWDR has been transmitted (TWEA = “0”); ACK has been received

// TWIQUEUE Slave Receiver status codes
#define TWIQUEUE_SRX_ADR_ACK            0x60  // Own SLA+W has been received ACK has been returned
#define TWIQUEUE_SRX_ADR_ACK_M_ARB_LOST 0x68  // Arbitration lost in SLA+R/W as Master; own SLA+W has been received; ACK has been returned
#define TWIQUEUE_SRX_GEN_ACK            0x70  // General call address has been received; ACK has been returned
#define TWIQUEUE_SRX_GEN_ACK_M_ARB_LOST 0x78  // Arbitration lost in SLA+R/W as Master; General call address has been received; ACK has been returned
#define TWIQUEUE_SRX_ADR_DATA_ACK       0x80  // Previously addressed with own SLA+W; data has been received; ACK has been returned
#define TWIQUEUE_SRX_ADR_DATA_NACK      0x88  // Previously addressed with own SLA+W; data has been received; NOT ACK has been returned
#define TWIQUEUE_SRX_GEN_DATA_ACK       0x90  // Previously addressed with general call; data has been received; ACK has been returned
#define TWIQUEUE_SRX_GEN_DATA_NACK      0x98  // Previously addressed with general call; data has been received; NOT ACK has been returned
#define TWIQUEUE_SRX_STOP_RESTART       0xA0  // A STOP condition or repeated START condition has been received while still addressed as Slave

// TWIQUEUE Miscellaneous status codes
#define TWIQUEUE_NO_STATE               0xF8  // No relevant state information available; TWIQUEUENT = “0”
#define TWIQUEUE_BUS_ERROR              0x00  // Bus error due to an illegal START or STOP condition
