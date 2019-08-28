/*
  Copyright (c) 2019 H.J.v.Aalderen.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include <bluefruit.h>
#include "pinIO.hpp"

const char* PIN_MODE_NAMES[6] = {"input","output","pullup","pulldown","analogIn","analogOut"};
#define PINMODE(mode) ( ((mode) < 6 && (mode) >=0) ? PIN_MODE_NAMES[mode] : "NONE" )

#ifndef Arduino_h
#define bitRead(value, bit) (((value) >> (bit)) & 0x01ul)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#endif


const uint8_t ReservedPins[] = {
//const std::vector<byte> ReservedPins = {
  0,1,           //  nc
  PIN_VBAT,      // ,31 ,A7
  //PIN_AREF,     // ,24
  //PIN_SERIAL_RX,  // ,8
  //PIN_SERIAL_TX,  // ,6
  //SS,         // ,18
  PIN_SPI_MISO, // ,14
  PIN_SPI_MOSI, // ,13
  PIN_SPI_SCK,    // ,12
  PIN_WIRE_SDA, // ,25
  PIN_WIRE_SCL, // ,26
  0xff
};
bool isReserved(byte pin) {
  for (byte i=0; ReservedPins[i]<0xff; i++) {
    if (ReservedPins[i] == pin )
      return true;
  }
  return false;
}

//#define IS_RESERVED(pin) ( (std::find(std::begin(ReservedPins), std::end(ReservedPins), pin) != std::end(ReservedPins)) )

byte pinIO::nPins;
pinIO * pinIO::pins; // required to repeat it ? by cpp
void pinIO::createDigBits(byte nBits)
{
  Serial.print("Setting up digital IO nr:"); Serial.println(nBits, DEC); 
  pins = new pinIO[nBits];
  for (byte pin=0; pin<nBits; pin++) {
    pins[pin].pin = (pin); //DIGIT_TO_PIN(pin);
    pins[pin].mode = IO_NONE;
  }
  pinIO::nPins = nBits;
}
bool pinIO::isDigital(byte pin) {
  for (byte p=0; p<nPins; p++)
     if (pin == pins[p].pin)
        return true;
  return false;
}

byte anaIO::nChans;
anaIO * anaIO::anachan;
void anaIO::createAnaPins(byte nPins)
{
  Serial.print("Setting up analog IO nr:"); Serial.println(nPins, DEC); 
  anachan = new anaIO [nPins];
  for (byte ach=0; ach<nPins; ach++) {
    anachan[ach].pin = (ANALOG_TO_PIN[ach]); 
    anachan[ach].mode = IO_NONE;
    anachan[ach].anaval = 0xffff;
  }
  nChans=nPins;
}
bool anaIO::isAnalog(byte pin) {
  for (byte p=0; p<nChans; p++)
     if (pin == anachan[p].pin && anachan[p].mode != IO_NONE)
        return true;
  return false;
}

// set pin mode to a digital (binary) pin (see pinIO.hpp)
void pinIO::setMode(byte mode) {
  if (this->mode != mode) {
   if (mode == IO_NONE) {
     Serial.print("disable dig mode on pin ");Serial.println(pin, DEC); 
     this->mode = mode;
   }
   else if (anaIO::isAnalog(pin)) {
     Serial.print("isAnalog pin ");Serial.println(pin, DEC); 
   } else {
   Serial.print("changing dig mode on pin ");Serial.print(pin, DEC);Serial.print(" to:"); Serial.print(mode, DEC); Serial.println(PINMODE(mode)); 
   switch (mode) {
    //case PIN_MODE_PWM:
    //  digitalWrite((pin), LOW);  // disable pullup
    case OUTPUT_ANALOG:
    case INPUT_ANALOG:
      if (anaIO::isAnalog(pin))
        this->mode = mode;  // reserve for analog
      break;
    case INPUT:
    case INPUT_PULLUP:
      pinMode(pin, mode);
      this->mode = mode;
      break;
    case OUTPUT:
      pinMode(pin, mode);
      this->mode = mode;
      break;
    default:
      Serial.println("...Unknown pin mode.");
      break;
    }
   }
  }
}

// set pin mode to analog channel
void anaIO::setMode(byte mode) {
  if (this->mode != mode) {
   Serial.print("changing ana mode on pin ");Serial.print(pin, DEC);Serial.print(" to:"); Serial.print(mode, DEC); Serial.println(PINMODE(mode)); 
   switch (mode) {
    //case PIN_MODE_PWM:
    //  digitalWrite(pin, LOW);  // disable pullup
    case INPUT_ANALOG:
    case INPUT:
      pinMode(pin, INPUT);
      this->mode=mode;   
      break;
    case INPUT_PULLUP:
      pinMode(pin, INPUT_PULLUP);
      this->mode=mode;
      break;
    default:
      Serial.println("..Unknown pin mode ");
      break;
   }
  }
}
// set pin value to bit
void pinIO::setState(bool bitv) {
  //if (IS_PIN_DIGITAL(pin))
    if (mode == OUTPUT) {  
      #ifdef DEBUG
      if (this->bitv != bitv)  // first time?
        Serial.print("Setting bit of pin:"); Serial.print(pin, DEC);Serial.print(" to:"); Serial.println(bitv, DEC);
      #endif
      digitalWrite(pin, bitv);
      this->bitv = bitv;
    }
}
word anaIO::getAnaState() {
  if (mode == INPUT_ANALOG || mode == INPUT_PULLUP)
     anaval = analogRead(pin);
  return anaval;
}
void anaIO::setAnaState(word value) {
  if (mode == OUTPUT_ANALOG)
     analogWrite((pin), value);  // however ... no an out pins
}

// accept bledat data from AIOS client 
bool pinIO::acceptBLEdig(uint8_t * bledat, uint8_t len){
  word changed = 0;
  for (uint8_t pini=0; pini<nPins && (pini >> 2)<len; pini++) {
      byte bit2 = (bledat[pini >>2] >> ((pini & 3)*2)) & 3;
      if (bit2 == 3){ 			// the unknown dig state
         Serial.print("ignoring pin:"); Serial.println(pins[pini].pin, DEC);
      } else if (isReserved(pins[pini].pin)) {
         Serial.print("rejected: RESERVED pin:"); Serial.println(pins[pini].pin, DEC);
      } else if (bit2 == 2) {	// defines the tri-state state
      	setMode(pini, INPUT_PULLUP);  //INPUT);
      	Serial.print("setting pin to input :"); Serial.println(pins[pini].pin, DEC);
      } else {
      	setMode(pini, OUTPUT);
      	//Serial.print("setting pin to output :"); Serial.println(pins[pini].pin, DEC);
      	bool bv = bit2 & 1;
      	if (getState(pini) != bv) {
      		setState(pini, bv);
      		changed++;
      	}
      }
   }
   return changed;
}

// produce digital data for aios client
word pinIO::produceBLEdig(uint8_t * bledat, uint8_t len){
  //byte cmp[nPins >> 2 + 1];
  word changed = 0;
  memset(bledat,0,len);
  for (uint8_t pini=0; pini<nPins; pini++) {
     bool bitv;
     if (pins[pini].mode == INPUT || pins[pini].mode == INPUT_PULLUP) {
       bitv = digitalRead(pins[pini].pin);
       if (pins[pini].bitv != bitv)
       	changed++;
       pins[pini].bitv = bitv;
       bledat[pini >> 2] |= 2 << ((pini & 3)*2);  // set input mode
     }else if(pins[pini].mode == OUTPUT){
       bitv = pins[pini].bitv;
     //}else if (pins[pini].mode == INPUT_ANALOG){
     }else{
       bitv = true;
       bledat[pini >> 2] |= 2 << ((pini & 3)*2);  // set tristate mode
     }
     bledat[pini >> 2] |= (bitv ? 1 : 0) << ((pini & 3)*2);
  }
  return changed;
}

// produce analog data for aios client
//
word anaIO::produceBLEana(byte chan){
  if (chan >= nChans)
    return 0xfffc;
  if (isReserved(anachan[chan].pin))
    return 0xfffd;
  if (anachan[chan].mode == IO_NONE)
    setMode(chan, INPUT_ANALOG);
  word newval = anachan[chan].getAnaState();
  //if (oldval == newval)
  //		return 0xffff;
  return newval;
}
