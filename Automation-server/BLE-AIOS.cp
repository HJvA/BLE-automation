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
#include <bluefruit.h>
#include "pinIO.hpp"
#include "BLE-AIOS.h"

#define ROUND_2_UINT(f) ((uint16_t)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))
//#define TIME_TRIG_COND (dig_time_trig & 0xf)   // little Endian?
//#define TIME_TRIG_VAL  (dig_time_trig >> 8)    // 

// structures for trigger conditions
time_trig_t ana_time_trigs[nAnaChan];
ana_val_trig_t ana_val_trigs[nAnaChan];
time_trig_t dig_time_trig;
//uint32_t dig_time_trig; // = 0x00000501;  // { .condition=1, .value=5 };
dig_val_trig_t dig_val_trig; // = { 0 };


BLEService   aios ;  // automation io severvice
BLECharacteristic digc ;  // digital 
BLECharacteristic anac[nAnaChan]; // analog

uint8_t  bps = 0;

// event : receiving something from client : settings : mode & output state
void write_callback(short unsigned int conn_hdl, BLECharacteristic* chr, uint8_t * datbuf, short unsigned int len){
  if (chr->uuid == digc.uuid){
    Serial.print("digital received: ");
    pinIO::acceptBLEdig(datbuf, len);
    for (int i=0;i<len;i++)
       Serial.print(datbuf[i], HEX);
   }else{
    Serial.print("analog received: ");
   }
}

// event handler 
void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value)
{
    Serial.print("CCCD Updated: ");
    Serial.print(cccd_value);
    Serial.println("");

    // Check the characteristic this CCCD update is associated with in case
    // this handler is used for multiple CCCD records.
    if (chr->uuid == anac[0].uuid) {
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.println("Measurement 'Notify' enabled");
        } else {
            Serial.println("Measurement 'Notify' disabled");
        }
    }
}

// Configure the automation io service
void setupAIOS(void) {
  pinIO::createDigBits(nDigBits);
  anaIO::createAnaPins(nAnaChan);

  Serial.print("Creating AIOS with GATT service:");Serial.println(UUID16_SVC_AUTOMATION_IO ,HEX);
  aios = BLEService(UUID16_SVC_AUTOMATION_IO);
  digc = BLECharacteristic(UUID16_CHR_DIGITAL);
  for (byte pin=0; pin<nDigBits; pin++) {
      if (isReserved(pin) == false) 	// aios skips reserved pins
        pinIO::setMode(pin, IO_NONE);
  }
  // setup default trigger conditions
  dig_time_trig = (time_trig_t) { 1, 5000U };  // { .condition=1, .value=5s };
  dig_val_trig = { 0x00 };  // { .condition=0, .mask={} };  // not used
  
  for (byte pini=0; pini<nAnaChan; pini++) {
    anac[pini] = BLECharacteristic(UUID16_CHR_ANALOG);  // chracteristic for each pin
    ana_time_trigs[pini] = { 1, 5 };		// not used
    ana_val_trigs[pini] = { 3, 10000 }; // only notify when step>1V i.e. default setting
  }
  
  Serial.print("Beginning AIOS service. LenDigBits="); Serial.print(LenDigBits, DEC); Serial.print(" nAnaChan="); Serial.println(nAnaChan,DEC);
  aios.begin();

  // digital chracteristic
  digc.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_WRITE_WO_RESP | CHR_PROPS_READ);
  digc.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  digc.setFixedLen(LenDigBits); 		// Alternatively .setMaxLen(uint16_t len);
  digc.setCccdWriteCallback(cccd_callback);  
  digc.setWriteCallback(write_callback);
  digc.begin();
  digc.addDescriptor(DSC_number_of_digitals, &nDigBits, sizeof(nDigBits), SECMODE_OPEN, SECMODE_NO_ACCESS);  // reserved bits and analog pins still have to subtracted
  digc.addDescriptor(DSC_time_trigger_setting, &dig_time_trig, sizeof(dig_time_trig), SECMODE_OPEN, SECMODE_OPEN);
 
  // analog chracteristics
  for (int ch=0; ch<nAnaChan; ch++) {
    anac[ch].setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
    anac[ch].setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    anac[ch].setFixedLen(2);
    // (uint8_t type , int8_t exponent, uint16_t unit, uint8_t name_space, uint16_t descritpor);
    anac[ch].setPresentationFormatDescriptor(DSC_data_type_uint16, -4, UN_voltage_volt, 0, ch); 
    anac[ch].begin();
    anac[ch].addDescriptor(DSC_value_trigger_setting, &ana_val_trigs[ch], sizeof(ana_val_trig_t), SECMODE_OPEN, SECMODE_OPEN); // only 1 setting supported now
  }
  //Serial.println("AIOS service been setup, starting to poll");
}

uint16_t mvolt[nAnaChan]; // stores latest analog values
// arduino keeps calling this when contacted
word pollAIOS(ulong trun){
	byte digdata[LenDigBits];
	word changed = pinIO::produceBLEdig(digdata, sizeof(digdata));
	if ( dig_time_trig.condition==1 && trun>dig_time_trig.tm.interv ){  // notification condition
		changed=1;
		digc.notify(digdata, sizeof(digdata));
		Serial.print("digital notified trun ms: "); Serial.println(trun, DEC); 
	}else if (changed) {  // just save to characterisric
		changed=0;
		digc.write(digdata, sizeof(digdata));
		Serial.println("updated digdata");
	}
    
	for (int chan = 0; chan < nAnaChan; chan++) {
		uint16_t ana = anaIO::produceBLEana(chan);
		if (ana < 0xfffe) {	// valid sample
			float volt = ana * 3.6 / 1023.0;  // analogReadResolution(10); analogReference(AR_DEFAULT);
			//float volt = ana * 3.6 / 4095.0;  // analogReadResolution(12); analogReference(AR_INTERNAL);
			//float volt = ana * 3.6 / 16383.0;  // analogReadResolution(14); analogReference(AR_INTERNAL);
         
			ana = ROUND_2_UINT(volt*10000);   // exponent -4 => 10000 as specified in descriptor
			if (ana_val_trigs[chan].condition==3 && ana_val_trigs[chan].lev.val<abs(ana-mvolt[chan])) {
				anac[chan].notify16(ana);
				Serial.print("analog");Serial.print(chan);Serial.print(" notified dev[V]: "); Serial.println(float(ana-mvolt[chan])/10000, DEC ); 
				mvolt[chan] = ana;
			}else{
				anac[chan].write16(ana);
			}
		}
	} 
	return changed;
}
