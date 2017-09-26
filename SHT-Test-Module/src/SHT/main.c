/*	
 * GccApplication2.c
 *
 * Created: 11/2/2016 8:11:54 PM
 * Author : Death
 */ 
#define F_CPU 8000000UL
#define DELAY _delay_ms(1)
#define SETCLK PORTC |= (1<<PORTC1)
#define CLRCLK PORTC &= ~(1<<PORTC1)
#define SETDTA PORTC |= (1<<PORTC0)
#define CLRDTA PORTC &= ~(1<<PORTC0)
#define GET_TEMP 0x03
#define GET_HUMID 0x04
#define PING_ON 0x01
#define PING_OFF 0x02
#include <avr/io.h>
#include <util/delay.h>
#include "i2c_packet.h"
#include <avr/interrupt.h>
static uint16_t tempC;
static uint16_t humidity;
uint16_t tempRaw;
int _numBits;
uint16_t readTemperatureRaw();
/**
 */

struct SHT_data{
    uint16_t _temperature;
    uint16_t humidity;
};

uint8_t shiftIn(int _numBits)
{
  uint8_t ret = 0;

  DDRC &= ~(1<<DDC0);
  DELAY;
  for (int i=0; i<_numBits; i++)
  {
	 SETCLK;
     DELAY;  // I don't know why I need this, but without it I don't get my 8 lsb of temp
     ret = ret*2 + (PINC & (1<<PINC0));
	 CLRCLK;
	 DELAY;
  }

  return(ret);
}

/**
 */
void shiftOut(uint8_t _command)
{
	DDRC |= (1<<DDC1);
	DDRC |= (1<<DDC0);
	
	for (int i = 0; i < 8; i++)
	{
		int output = 0;
		output = _command & 0b10000000;
		_command = _command << 1;
		if(output)
		{
			SETDTA;
			DELAY;
		}
		else
		{
			CLRDTA;
			DELAY;
		}
		SETCLK;
		DELAY;
		CLRCLK;
		DELAY;
	}
}

/**
 */
void sendCommandSHT(uint8_t _command)
{
  uint8_t ack;

  // Transmission Start
  CLRCLK;
  DELAY;
  SETDTA;
  DELAY;
  DDRC |= (1<<DDC1);
  DDRC |= (1<<DDC0);
  DELAY;
  SETCLK;
  DELAY;
  CLRDTA;
  DELAY;
  CLRCLK;
  DELAY;
  SETCLK;
  DELAY;
  SETDTA;
  DELAY;
  CLRCLK;
  DELAY;
  CLRDTA;
  DELAY;

  // The command (3 msb are address and must be 000, and last 5 bits are command)
  shiftOut(_command);

  // Verify we get the correct ack
  DDRC &= ~(1<<DDC0);
  DELAY;
  SETCLK;

  ack = PINC & (1<<PINC0);
  DELAY;
  if (ack != 0) {
    /*return;*/
  }
  
  CLRCLK;
  DELAY;
  ack = PINC & (1<<PINC0);
  if (ack != 1) {
    /*return;*/
  }
  //TODO: Make the code handle ACK
}

/**
 */
void waitForResultSHT()
{
  int i;
  int ack;
  
  DDRC &= ~(1<<DDC0);

  for(i= 0; i < 200; ++i)
  {
    _delay_ms(2);
    ack = PINC & (1<<PINC0);

    if (ack == 0) {
      break;
    }
  }

  if (ack == 1) {
    /*return;*/
  }
}

/**
 */
uint16_t getData16SHT()
{
  uint16_t val;

  // Get the most significant bits
  DDRC &= ~(1<<DDC0);
  DDRC |= (1<<DDC1);
  val = shiftIn(8);
  val *= 256;

  // Send the required ack
  DDRC |= (1<<DDC0);
  DELAY;
  SETDTA;
  DELAY;
  CLRDTA;
  DELAY;
  SETCLK;
  DELAY;
  CLRCLK;
  DELAY;

  // Get the least significant bits
  DDRC &= ~(1<<DDC0);
  
  val |= shiftIn(8);

  return val;
}

/**
 */
void skipCrcSHT()
{
  // Skip acknowledge to end trans (no CRC)
  DDRC |= (1<<DDC0);
  DDRC |= (1<<DDC1);
  
  SETDTA;
  DELAY;
  SETCLK;
  DELAY;
  CLRCLK;
  DELAY;
}

/**
 * Reads the current temperature in degrees Celsius
 */
float readTemperatureC()
{
  float _val;                // Raw value returned from sensor
  float _temperature;      // Temperature derived from raw value

  // Conversion coefficients from SHT15 datasheet
  const float D1 = -40.1;  // for 14 Bit @ 5V
  const float D2 =   0.01; // for 14 Bit DEGC

  // Fetch raw value
  _val = readTemperatureRaw();

  // Convert raw value to degrees Celsius
  _temperature = (_val * D2) + D1;
  
  return (_temperature);
}

/**
 * Reads the current temperature in degrees Fahrenheit
 */
float readTemperatureF()
{
  uint16_t _val;                 // Raw value returned from sensor
  float _temperature;       // Temperature derived from raw value

  // Conversion coefficients from SHT15 datasheet
  const float D1 = -40.0;   // for 14 Bit @ 5V
  const float D2 =   0.018; // for 14 Bit DEGF

  // Fetch raw value
  _val = readTemperatureRaw();

  // Convert raw value to degrees Fahrenheit
  _temperature = (((float)_val) * D2) + D1;

  return (_temperature);
}

/**
 * Reads current temperature-corrected relative humidity
 */
float readHumidity()
{
  float _val;                    // Raw humidity value returned from sensor
  float _linearHumidity;       // Humidity with linear correction applied
  float _correctedHumidity;    // Temperature-corrected humidity
  float _temperature;          // Raw temperature value

  // Conversion coefficients from SHT15 datasheet
  const float C1 = -4.0;       // for 12 Bit
  const float C2 =  0.0405;    // for 12 Bit
  const float C3 = -0.0000028; // for 12 Bit
  const float T1 =  0.01;      // for 14 Bit @ 5V
  const float T2 =  0.00008;   // for 14 Bit @ 5V

  // Command to send to the SHT1x to request humidity
  int _gHumidCmd = 0b00000101;

  // Fetch the value from the sensor
  sendCommandSHT(_gHumidCmd);
  waitForResultSHT();
  _val = getData16SHT();
  skipCrcSHT();

  // Apply linear conversion to raw value
  _linearHumidity = C1 + C2 * _val + C3 * _val * _val;

  // Get current temperature for humidity correction
  _temperature = readTemperatureC();

  // Correct humidity value for current temperature
  _correctedHumidity = (_temperature - 25.0 ) * (T1 + T2 * _val) + _linearHumidity;

  return (_correctedHumidity);
}

/**
 * Reads the current raw temperature value
 */
uint16_t readTemperatureRaw()
{
  uint16_t _val;

  // Command to send to the SHT1x to request Temperature
  int _gTempCmd  = 0b00000011;

  sendCommandSHT(_gTempCmd);
  waitForResultSHT();
  _val = getData16SHT();
  skipCrcSHT();

  return (_val);
}
int main(void)
{
	_delay_ms(20);
	DDRB = 0b00010011;
	DDRC = 0b00000010;
	DDRD = 0b11000111;
	PORTC &= ~(1<<PORTC1);
	i2c_init(77);
	sei();
	int counter = 0;
	struct packet receivedPacket;
	struct packet returnPacket;
	returnPacket.cmd = 77;
	returnPacket.buffer[0] = 11;
	returnPacket.buffer[1] = 11;
	returnPacket.buffer[2] = 11;
	returnPacket.buffer[3] = 11;
		
    while (1) 
    {
			
        i2c_checkForPackets();
        i2c_setReturnPacket(&returnPacket, 2);
        if(i2c_hasPacket())
		{		
			i2c_getPacket(&receivedPacket);
			PORTD |= (1<<PORTD2);
			switch(receivedPacket.cmd)
			{	
				case PING_ON:
					PORTD |= (1<<PORTD1);
					break;

				case PING_OFF:
					PORTD &= ~(1<<PORTD1);
					break;	
		
				case GET_TEMP:
 				    tempC = (uint16_t)(readTemperatureC()*100);
 				    returnPacket.cmd = GET_TEMP;
 				    returnPacket.buffer[0] = (uint8_t) (tempC);
 				    returnPacket.buffer[1] = (uint8_t) (tempC>>8);
				    break;

				case GET_HUMID:
 				    humidity = (uint16_t)(readHumidity()*100);
 				    returnPacket.cmd = GET_HUMID;
 				    returnPacket.buffer[0] = (uint8_t) (humidity);
 				    returnPacket.buffer[1] = (uint8_t) (humidity>>8);
				    break;

 				default:
 					PORTD ^= (1<<PORTD0);
 					break;
        	}
        }
       	_delay_ms(1);
	    counter++;
        if(counter>=1){
            counter = 0;
            PORTD ^= (1<<PORTD0);
        }
    }
}
