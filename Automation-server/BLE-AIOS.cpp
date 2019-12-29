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
#define anaSCL 10000

// structures for trigger conditions
time_trig_t ana_time_trigs[nAnaChan];
ana_val_trig_t ana_val_trigs[nAnaChan];
//ana_valid_range_t ana_valid_range[nAnaChan];
time_trig_t dig_time_trig;
//uint32_t dig_time_trig; // = 0x00000501;  // { .condition=1, .value=5 };
dig_val_trig_t dig_val_trig; // = { 0 };

anaCharacteristic::anaCharacteristic() : BLECharacteristic(){}
anaCharacteristic::anaCharacteristic(BLEUuid bleuuid, uint8_t chan) : BLECharacteristic(bleuuid)
{
		
    anachan = chan; 
}
// this one returning handle
uint16_t add_Descriptor(BLEUuid bleuuid, void const * content, uint16_t len, BleSecurityMode read_perm, BleSecurityMode write_perm)
{
  // Meta Data
  ble_gatts_attr_md_t meta;
  varclr(&meta);
  memcpy(&meta.read_perm , &read_perm , 1);
  memcpy(&meta.write_perm, &write_perm, 1);
  meta.vlen = 0;
  meta.vloc = BLE_GATTS_VLOC_STACK;

  // Descriptor
  (void) bleuuid.begin();

  ble_gatts_attr_t desc =
  {
      .p_uuid    = &bleuuid._uuid,
      .p_attr_md = &meta,
      .init_len  = len,
      .init_offs = 0,
      .max_len   = len,
      .p_value   = (uint8_t*) content
  };
  uint16_t hdl;
  sd_ble_gatts_descriptor_add(BLE_GATT_HANDLE_INVALID, &desc, &hdl);
  return hdl;
}
void anaCharacteristic::addDescriptors(void){
    char buf [5];
    sprintf(buf, "ch%d", anachan);
    setUserDescriptor(buf); // alternative to recognise indiv chans
    add_Descriptor(DSC_value_trigger_setting, &ana_val_trigs[anachan], sizeof(ana_val_trig_t),
		  SECMODE_OPEN, SECMODE_OPEN); // only 1 setting supported now
			
    ana_valid_range_t ana_valid_range = { 0, volt2anaDat(anaIO::maxVoltRange(anachan), anaSCL) };
    //uint32_t, sd_ble_gatts_descriptor_add(uint16_t char_handle, ble_gatts_attr_t const *p_attr, uint16_t *p_handle));
    valrngHandle = add_Descriptor(DSC_valid_range, &ana_valid_range, sizeof(ana_valid_range_t),
		  SECMODE_OPEN, SECMODE_OPEN); // also writable here !
    //Serial.print("chan "); Serial.println(_format_desc.desc);
}
void anaCharacteristic::updateVoltRange(void) {
	ana_valid_range_t ana_valid_range;
	ble_gatts_value_t value =
  	{
      .len     = sizeof(ana_valid_range_t),
      .offset  = 0,
      .p_value = (uint8_t*) &ana_valid_range
 		};

  // conn handle only needed for system attribute
  sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, valrngHandle, &value);
	
	anaIO::setReference(anachan, ana_valid_range.Upper_inclusive_value /(anaSCL/1000));  
  Serial.print("set VoltRng on:");Serial.print(anachan); Serial.print(" to [mV]:"); Serial.println(ana_valid_range.Upper_inclusive_value /(anaSCL/1000));
}

BLEService   aios ;  // automation io severvice
BLECharacteristic digc ;  // digital 
anaCharacteristic anac[nAnaChan]; // analog

// event : receiving something from client : settings : mode & output state
void write_callback(short unsigned int conn_hdl, BLECharacteristic* chr, uint8_t * datbuf, short unsigned int len){
  if (chr->uuid == digc.uuid){
    Serial.print("digital received: ");
    pinIO::acceptBLEdig(datbuf, len);
    for (int i=0;i<len;i++)
       Serial.print(datbuf[i], HEX);
  } else if (chr->uuid == BLEUuid(DSC_valid_range)) {  // condition never met
    Serial.print("analog valid range received: ");
    ana_valid_range_t * vrrec =(ana_valid_range_t *)datbuf;
    Serial.println(vrrec->Upper_inclusive_value);
    byte chIdx = vrrec->Lower_inclusive_value;  // misusing this for fetching channel
    anaIO::setReference(chIdx , vrrec->Upper_inclusive_value /(anaSCL/1000));
    //Serial.print(chr->_format_desc);
   }else{
    Serial.print("unknown received: ");
   }
}

// event handler Client Characteristic Configuration Descriptor updated
void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value)
{
    Serial.print("CCCD Updated val=");
    Serial.print(cccd_value);
    Serial.print(" uuid sz:");
    Serial.println(chr->uuid.size());

    if (chr->uuid == digc.uuid)
       Serial.println("Digital notifying ?");
    if (chr->uuid == anac[0].uuid) {  // starting notification on anachan
         //anaCharacteristic * ana = dynamic_cast<anaCharacteristic *>(chr);
         anaCharacteristic * ana = (anaCharacteristic *) chr;
         byte ch = ana->anachan;  
         if (anaIO::setMode(ch, INPUT_ANALOG))  // activate the channel
           delay(100);

				 anac[ch].updateVoltRange();
        if (chr->notifyEnabled(conn_hdl)) {
            Serial.print("enabling");
        } else {
            Serial.print("disabled");
        }
        Serial.print(" notify on analog chan:");
        Serial.println(ch);
    }
}

// time difference between first tick1 and second tick2
ulong tdiff(ulong tick1, ulong tick2)
{
	if (tick2 > tick1)
		return tick2-tick1;
	return ULONG_MAX - tick1 + tick2;
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
  // setup default trigger conditions (at least every 5 minutes)
  dig_time_trig = (time_trig_t) { ttINTERVAL, millis(), 300000U };  // { .condition=1, .tookms=0, .value=5s };
  dig_val_trig = { 0x00 };  // { .condition=0, .mask={} };  // not used
  
  for (byte pini=0; pini<nAnaChan; pini++) {
    anac[pini] = anaCharacteristic(UUID16_CHR_ANALOG, pini);  // chracteristic for each pin
    ana_time_trigs[pini] = { ttINTERVAL,0, 60000U };		// 
    ana_val_trigs[pini] = { vtDEVIATES, 1*anaSCL }; // only notify when step>1V i.e. default setting
  }
  
  Serial.print("Beginning AIOS service. LenDigBits="); Serial.print(LenDigBits, DEC); Serial.print(" bytes nAnaChan="); Serial.println(nAnaChan,DEC);
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
		anac[ch].setWriteCallback(write_callback);
    anac[ch].setCccdWriteCallback(cccd_callback);
    // (uint8_t type , int8_t exponent, uint16_t unit, uint8_t name_space, uint16_t descritpor);
    // NOTE also used to recognise individual channels
    anac[ch].setPresentationFormatDescriptor(DSC_data_type_uint16, -4, UN_voltage_volt, 0, ch); 
    anac[ch].begin();
    anac[ch].addDescriptors();
  }
  //Serial.println("AIOS service been setup, starting to poll");
}

uint16_t takeAnaGatt(int chan) {
	uint16_t ana = anaIO::produceBLEana(chan, anaSCL);
	return ana;
}

uint16_t mvolt[nAnaChan]; // stores latest analog values

// arduino keeps calling this when contacted
word pollAIOS(ulong tick){
	byte digdata[LenDigBits];
	word changed = pinIO::produceBLEdig(digdata, sizeof(digdata));
	ulong trun = tdiff(dig_time_trig.tookms, tick);
	if (digc.notifyEnabled()) {
		if ( changed || (dig_time_trig.condition==ttINTERVAL && trun>dig_time_trig.tm.interv) ){  // notification condition
			changed=1;
			digc.notify(digdata, sizeof(digdata));
			Serial.print("digital notified trun ms: "); Serial.println(trun, DEC); 
			dig_time_trig.tookms = tick;
		} else {
			digc.write(digdata, sizeof(digdata));
		}
	} else if (changed) {  // just save to characterisric
		changed=0;
		digc.write(digdata, sizeof(digdata));
		Serial.println(" updated digdata");
	}
    
	for (int chan = 0; chan < nAnaChan; chan++) {
		trun = tdiff(ana_time_trigs[chan].tookms, tick);
		uint16_t ana = takeAnaGatt(chan);  // anaIO::produceBLEana(chan);
		if (ana < 0xfff0) {	// valid sample
			if (anac[chan].notifyEnabled()) {
        Serial.print(" analog chan:");Serial.print(chan);
				if ((ana_val_trigs[chan].condition==vtDEVIATES && ana_val_trigs[chan].lev.val<abs(ana-mvolt[chan])) ||
					(ana_time_trigs[chan].condition==ttINTERVAL && trun>ana_time_trigs[chan].tm.interv)) {
					anac[chan].notify16(ana);
					Serial.print(" trun ms:"); Serial.print(trun, DEC);
					Serial.print(" notified [V]:"); 
					ana_time_trigs[chan].tookms = tick;
					mvolt[chan] = ana;
					changed++;
				} else {
					anac[chan].write16(ana);
          Serial.print(" [V]:");  
				}
        Serial.print(float(ana)/anaSCL, DEC );
			} else if (anaIO::getMode(chan) == INPUT_ANALOG) {
				uint16_t ana = takeAnaGatt(chan);
				if (ana < 0xfff0) {	// valid sample
					anac[chan].write16(ana);  // saving to characteristic
				}
			}
		}
	} 
	return changed;
}
