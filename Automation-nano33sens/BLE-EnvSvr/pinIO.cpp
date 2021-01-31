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
#include <math.h>
#include "pinIO.h"

const char* PIN_MODE_NAMES[6] = {"input","output","pullup","pulldown","analogIn","analogOut"};

#define DEBUG 1
#define PINMODE(mode) ( ((mode) < 6 && (mode) >=0) ? PIN_MODE_NAMES[mode] : "NONE" )

#ifndef Arduino_h
#define bitRead(value, bit) (((value) >> (bit)) & 0x01ul)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#endif

#define ROUND_2_UINT(f) ((uint16_t)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

// reserved pins on device x
// adafruit feather
// arduino nano 
const uint8_t ReservedPins[] = {
  0,1,           //  nc
  #ifdef BLUEFRUIT_H_
  PIN_VBAT,      // ,31 ,A7
  #endif
  //PIN_AREF,       // ,24
  PIN_SERIAL_RX,  // ,8
  PIN_SERIAL_TX,  // ,6
  //SS,           // ,18
  PIN_SPI_MISO,   // ,14
  PIN_SPI_MOSI,   // ,13
  //PIN_SPI_SCK,    // ,Led
  PIN_WIRE_SDA,   // ,25
  PIN_WIRE_SCL,   // ,26
  0xff
};
// check whether pin is not available (F)
bool isReserved(byte pin) {
  for (byte i=0; ReservedPins[i]<0xff; i++) {
    if (ReservedPins[i] == pin )
      return true;
  }
  return false;
}

// time difference between first tick1 and second tick2
ulong tdiff(ulong tick1, ulong tick2)
{
	if (tick2 > tick1)
		return tick2-tick1;
	return ULONG_MAX - tick1 + tick2;
}

//#define IS_RESERVED(pin) ( (std::find(std::begin(ReservedPins), std::end(ReservedPins), pin) != std::end(ReservedPins)) )

#if BLUEFRUIT_H_
//#define refADC eAnalogReference
eAnalogReference ADCREFMODES[NREFS] {
    AR_INTERNAL_1_2,
    AR_INTERNAL_1_8,
    AR_INTERNAL_2_4,
    AR_INTERNAL_3_0,
    AR_INTERNAL,
    AR_VDD4
 };

word mVREF[NREFS] = {  // reference voltages for analog inp
    1200,
    1800,
    2400,
    3000,
    3600,
    5000
};
#else   // nano33
//#define refADC  _AnalogReferenceMode
 _AnalogReferenceMode ADCREFMODES[NREFS] {                                                                          
  AR_INTERNAL,    // 0.6 V                                                                             
  AR_INTERNAL1V2, // 1.2 V                                                                             
  AR_INTERNAL2V4,  // 2.4 V 
  AR_VDD        // 3.3 V   
};
word mVREF[NREFS] = {  // reference voltages for analog inp
     600,
    1200,
    2400,
    3300
 };

//const byte LED_RED = LED_BUILTIN;
//#define LED_RED LED_PWR
//LED_BUILTIN
#endif

// ****** binary IO bits (pinIO class) *********
byte pinIO::nPins;
pinIO * pinIO::pins; // required to repeat it ? by cpp
// create all bits, mapping bitidx 1:1 to pin
void pinIO::createDigBits(byte nBits)
{
  Serial.print("Setup pinIO nbits:"); Serial.println(nBits, DEC); 
  pins = new pinIO[nBits];
  for (byte iBit=0; iBit<nBits; iBit++) {
    pins[iBit].pin = (iBit); //DIGIT_TO_PIN(pin);
    pins[iBit].mode = IO_NONE;
  }
  pinIO::nPins = nBits;
}
// T : possible digital pin
bool pinIO::isDigital(byte pin) {
  for (byte iBit=0; iBit<nPins; iBit++)
     if (pin == pins[iBit].pin)
        return true;
  return false;
}

// set pin mode to a digital (binary) pin (see pinIO.hpp) returns success
bool pinIO::setMode(byte mode) {
  if (this->mode != mode) {
   if (mode == IO_NONE) {
     Serial.print("disable dig mode on pin ");Serial.println(pin, DEC); 
     this->mode = mode;
   }
   else if (anaIO::isAnalog(pin)) {
     Serial.println("ana pin can not be dig !! ");Serial.println(pin, DEC); 
     return false;
   } else {
     Serial.print("changing dig mode on pin ");Serial.print(pin, DEC);Serial.print(" to: "); Serial.println(PINMODE(mode)); 
     if (isReserved(pin) || !isDigital(pin)) {
       Serial.print("not allowed !! for pin:");Serial.println(pin);
       return false;
     }
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
      return false;
     }
   }
  }
  return true;
}

// set pin value to bit
void pinIO::setState(bool bitv) {
  //if (IS_PIN_DIGITAL(pin))
    if (mode == OUTPUT) {  
      #ifdef DEBUG
      if (this->bitv != bitv) { // first time?
        Serial.print("changing bit state of pin:"); Serial.print(pin, DEC);Serial.print(" to:"); Serial.println(bitv, DEC);
        }
      #endif
      digitalWrite(pin, bitv);
      this->bitv = bitv;
    }
}

// ******* analog input channels (anaIO class) *********
byte anaIO::nChans;
anaIO * anaIO::anachans;
void anaIO::createAnaPins(byte nPins)
{
  Serial.print("Setting up analog IO nr:"); Serial.println(nPins, DEC); 
  anachans = new anaIO [nPins];
  for (byte ach=0; ach<nPins; ach++) {
    anachans[ach].pin = (ANALOG_TO_PIN[ach]); 
    anachans[ach].mode = IO_NONE;  // not active by default
    anachans[ach].anaval = 0xffff;
	#ifdef BLUEFRUIT_H_
	 anachans[ach].reference = refADC::v36;
	 anachans[ach].resolution = 12;
	#else
	 anachans[ach].reference = refADC::vVDD;  // 3.3V
    anachans[ach].resolution = 10;  //ADC_RESOLUTION;
	 analogReadResolution(10);
	#endif
	 Serial.print("aChan:");Serial.print(ach);Serial.print(" pin:");Serial.print(anachans[ach].pin);
	 Serial.print(" Vrng:");Serial.println(anachans[ach].maxVoltRange());
  }
  nChans=nPins;
}

// T for active analog pin
bool anaIO::isAnalog(byte pin) {
  for (byte p=0; p<nChans; p++)
     if (pin == anachans[p].pin && anachans[p].mode != IO_NONE)
        return true;
  return false;
}

// set pin mode to analog channel
bool anaIO::setMode(byte mode) {
  if (this->mode != mode) {
   Serial.print("changing ana mode on pin ");Serial.print(pin, DEC);Serial.print(" to:"); Serial.print(mode, DEC); Serial.println(PINMODE(mode)); 
   if (isReserved(pin)) {
     Serial.println("reseved pin !!");
     return false;
   }
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
      return false;;
    }
   }
  return true;
}

uint16_t anaIO::getAnaState() {
  if (mode == INPUT_ANALOG || mode == INPUT_PULLUP)
     anaval = analogRead(pin);
  return anaval;
}
void anaIO::setAnaState(uint16_t value) {
  if (mode == OUTPUT_ANALOG)
     analogWrite((pin), value);  // however ... no analog out pins
}
bool anaIO::setReference(word mVref ) {
	bool chg = false;
	if (setMode(INPUT_ANALOG)) { 
		word ref;
		for (ref=0; ref<NREFS && mVREF[ref]<mVref; ref++) {}
		chg = (ref<NREFS && reference != (refADC)ref);
		if (chg){
			Serial.print("set ana reference on pin:");Serial.print(pin);Serial.print(" lev:");Serial.println(ref);
			analogReference(ADCREFMODES[ref]); 
			reference=(refADC)ref;
		}
	}
	return chg;
}
void anaIO::setResolution(byte nbits) {
	resolution = nbits;
	analogReadResolution(nbits);
}
float anaIO::maxVoltRange() {
	return float(mVREF[(int)reference])/1000.0;
}

float anaIO::getVoltage() {
	word anas = getAnaState();
	if (anas < 0xfff0){
    float refV = maxVoltRange();
    word cnts = 1 << resolution;
		return anas*refV/cnts;
	}
	return NAN;
}

word volt2anaDat(float volt, uint16_t scale) {
  return ROUND_2_UINT( volt*scale );  //exponent -4 => 10000 as specified in descriptor
}
float anaDat2volt(uint16_t anaDat, uint16_t scale) {
  if (anaDat<0xfff0)
    return  float(anaDat)/scale ;  //exponent -4 => 10000 as specified in descriptor
  return NAN;
}

// produce analog data for aios client
// error condition if result > 0xfff0
uint16_t anaIO::produceBLEana(byte chan, uint16_t scale) {
  if (chan >= nChans)
    return 0xfffc;  
  if (isReserved(anachans[chan].pin))
    return 0xfffd;
  
  if (anachans[chan].mode == IO_NONE) {
    //  setMode(chan, INPUT_ANALOG);
    //Serial.print(chan,DEC);Serial.println(' ana not set to input');
    return 0xfffb;  //anachan[chan].anaval;
  }
  float newval = anachans[chan].getVoltage();  //getAnaState();
  if (isnan(newval))
    return 0xfffa;
  return volt2anaDat(newval, scale);
}
// accept bledat data from AIOS client 
bool pinIO::acceptBLEdig(uint8_t * bledat, uint8_t len){
  word changed = 0;
  for (uint8_t iBit=0; iBit<nPins && (iBit >> 2)<len; iBit++) {
      byte bit2 = (bledat[iBit >>2] >> ((iBit & 3)*2)) & 3;
      if (bit2 == 3){ 			// the unknown dig state
         #ifdef DEBUG
         Serial.print("ignoring pin:"); Serial.println(pins[iBit].pin, DEC);
         #endif
      } else if (isReserved(pins[iBit].pin)) {
         Serial.print("rejected: RESERVED pin:"); Serial.println(pins[iBit].pin, DEC);
      } else if (bit2 == 2) {	// defines the tri-state state
      	setMode(iBit, INPUT_PULLUP);  //INPUT);
      	//Serial.print("setting pin to input :"); Serial.println(pins[iBit].pin, DEC);
      } else if (setMode(iBit, OUTPUT)) {
      	//Serial.print("setting pin to output :"); Serial.println(pins[pini].pin, DEC);
      	bool bv = bit2 & 1;
      	if (getState(iBit) != bv) {
      		setState(iBit, bv);
      		changed++;
      	}
      }
   }
   return changed;
}

// produce digital data for aios client
// returns nr of input bits that have changed
word pinIO::produceBLEdig(uint8_t * bledat, uint8_t len){
  word changed = 0;
  memset(bledat,0,len);
  for (uint8_t iBit=0; iBit<nPins; iBit++) {
     bool bitv;
     if (pins[iBit].mode == INPUT || pins[iBit].mode == INPUT_PULLUP) {
       bitv = digitalRead(pins[iBit].pin);
       if (pins[iBit].bitv != bitv)
       	changed++;
       pins[iBit].bitv = bitv;
       bledat[iBit >> 2] |= 2 << ((iBit & 3)*2);  // set input mode
     }else if(pins[iBit].mode == OUTPUT){
       bitv = pins[iBit].bitv;
     //}else if (pins[iBit].mode == INPUT_ANALOG){
     }else{
       bitv = true;
       bledat[iBit >> 2] |= 2 << ((iBit & 3)*2);  // set tristate mode
     }
     bledat[iBit >> 2] |= (bitv ? 1 : 0) << ((iBit & 3)*2);
  }
  return changed;
}


