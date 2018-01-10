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
#ifndef SHT1x_H
#define SHT1x_H
#include <avr/io.h>
#include <util/delay.h>
#define DELAY (_delay_ms(10))
#define SETCLK (PORTC |= (1<<PORTC1))
#define CLRCLK (PORTC &= ~(1<<PORTC1))
#define SETDTA (PORTC |= (1<<PORTC0))
#define CLRDTA (PORTC &= ~(1<<PORTC0))

uint16_t readHumidity();
uint16_t readTemperatureC();
float readTemperatureF();

float readTemperatureRaw();
int shiftIn(int _numBits);
void sendCommandSHT(int _command);
void shiftOut(uint8_t _command);
void waitForResultSHT();
int getData16SHT();
void skipCrcSHT();

#endif
