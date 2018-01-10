/**
 * SHT1x Library
 *
 * Copyright 2009 Jonathan Oxer <jon@oxer.com.au> / <www.practicalarduino.com>
 * Based on previous work by:
 *    Maurice Ribble: <www.glacialwanderer.com/hobbyrobotics/?p=5>
 *    Wayne ?: <ragingreality.blogspot.com/2008/01/ardunio-and-sht15.html>
 *
 * Manages communication with SHT1x series (SHT10, SHT11, SHT15)
 * temperature / humidity sensors from Sensirion (www.sensirion.com).
 */
#include "SHT1x.h"

/* ================  Public methods ================ */

/**
 * Reads the current temperature in degrees Celsius
 */

uint16_t readTemperatureC()
{
  int _val;                // Raw value returned from sensor
  float _temperature;      // Temperature derived from raw value

  // Conversion coefficients from SHT15 datasheet
  const float D1 = -40.0;  // for 14 Bit @ 5V
  const float D2 =   0.01; // for 14 Bit DEGC

  // Fetch raw value
  _val = readTemperatureRaw();

  // Convert raw value to degrees Celsius
  _temperature = (_val * D2) + D1;
  //_temperature *= 100;

  return ((uint16_t)_temperature);
}

  /**
   * Reads the current temperature in degrees Fahrenheit
   */
float readTemperatureF()
{
  int _val;                 // Raw value returned from sensor
  float _temperature;       // Temperature derived from raw value

  // Conversion coefficients from SHT15 datasheet
  const float D1 = -40.0;   // for 14 Bit @ 5V
  const float D2 =   0.018; // for 14 Bit DEGF

  // Fetch raw value
  _val = readTemperatureRaw();

  // Convert raw value to degrees Fahrenheit
  _temperature = (_val * D2) + D1;

  return (_temperature);
}

/**
 * Reads current temperature-corrected relative humidity
 */
uint16_t readHumidity()
{
  int _val;                    // Raw humidity value returned from sensor
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
  sendCommandSHT(_gHumidCmd );
  waitForResultSHT();
  _val = getData16SHT();
  skipCrcSHT();

  // Apply linear conversion to raw value
  _linearHumidity = C1 + C2 * _val + C3 * _val * _val;

  // Get current temperature for humidity correction
  _temperature = readTemperatureC();

  // Correct humidity value for current temperature
  _correctedHumidity = (_temperature - 25.0 ) * (T1 + T2 * _val) + _linearHumidity;
  _correctedHumidity *= 100;
  return ((uint16_t)_correctedHumidity);
}


/* ================  Private methods ================ */

/**
 * Reads the current raw temperature value
 */
float readTemperatureRaw()
{
  int _val;

  // Command to send to the SHT1x to request Temperature
  int _gTempCmd  = 0b00000011;

  sendCommandSHT(_gTempCmd);
  waitForResultSHT();
  _val = getData16SHT();
  skipCrcSHT();

  return (_val);
}

/**
 */
int shiftIn(int _numBits)
{
  int ret = 0;

  for (int i = 0; i < _numBits; i++)
  {
    SETCLK;
    DELAY;
    ret = ret*2 + (PINC & (1<<PINC0));
    CLRCLK;
  }

  return(ret);
}

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
    }
    else
    {
      CLRDTA;
    }
    SETCLK;
    CLRCLK;
  }
}

/**
 */
void sendCommandSHT(int _command)
{
  int ack;

  // Transmission Start
  DDRC |= (1<<DDC1);
  DDRC |= (1<<DDC0);
  SETDTA;
  SETCLK;
  CLRDTA;
  CLRCLK;
  SETCLK;
  SETDTA;
  CLRCLK;

  // The command (3 msb are address and must be 000, and last 5 bits are command)
  shiftOut(_command);

  // Verify we get the correct ack
  DDRC &= ~(1<<DDC0);
  SETCLK;

  ack = PINC & (1<<PINC0);
}

/**
 */
void waitForResultSHT()
{
  int i;
  int ack;

  for(i= 0; i < 100; ++i)
  {
    _delay_ms(10);
    ack = PINC & (1<<PINC0);

    if (ack == 0) {
      break;
    }
  }

  if (ack == 1) {
    //Serial.println("Ack Error 2"); // Can't do serial stuff here, need another way of reporting errors
  }
}

/**
 */
int getData16SHT()
{
  uint16_t val;

  // Get the most significant bits
  DDRC &= ~(1<<DDC0);
  DDRC |= (1<<DDC1);
  val = shiftIn(8);
  val *= 256;

  // Send the required ack
  DDRC |= (1<<DDC0);
  SETDTA;
  CLRDTA;
  SETCLK;
  CLRCLK;

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
  SETCLK;
  CLRCLK;
}