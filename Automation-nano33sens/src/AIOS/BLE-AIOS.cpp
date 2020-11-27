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
#include <ArduinoBLE.h>
#include "pinIO/pinIO.h"
#include "BLE-AIOS.h"

#define ROUND_2_UINT(f) ((uint16_t)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))
#define anaSCALE 10000	// exponent -4 in PresentationFormatDescriptor => 10000

		time_trig_t dig_time_trig;
		dig_val_trig_t dig_val_trig; 
		
#ifdef ANAS
// structures for trigger conditions
time_trig_t ana_time_trigs[nAnaChan];
ana_val_trig_t ana_val_trigs[nAnaChan];
#endif
//ana_valid_range_t ana_valid_range[nAnaChan];

#ifdef BLUEFRUIT_H_

#else  // Arduino_h 
  void on_digc_written(BLEDevice central, BLECharacteristic chr){
	//if (chr.uuid == digc.uuid){
    Serial.print("digital received: ");
	 byte datbuf[256];
	 int len = chr.readValue(datbuf, 256);
    pinIO::acceptBLEdig(datbuf, len);
    for (int i=0;i<len;i++)
       Serial.print(datbuf[i], HEX);
  }
  //digCharacteristic::digCharacteristic() : BLECharacteristic("",0,""){}
  digCharacteristic::digCharacteristic(char* bleuuid) : 
     BLECharacteristic(bleuuid, BLERead | BLEWrite | BLENotify, LenDigBits, true) 
  { 
	// setup default trigger conditions (at least every 5 minutes)
	dig_time_trig = (time_trig_t) { ttINTERVAL, millis(), 300000U };  // { .condition=1, .tookms=0, .value=5s };
	dig_val_trig = { 0x00 };  // { .condition=0, .mask={} };  // not used
	Serial.print(" LenDigBits="); Serial.print(LenDigBits, DEC);
	
	BLEDescriptor dsc(DSC_number_of_digitals, &nDigBits, sizeof(nDigBits));
	addDescriptor(dsc);
	BLEDescriptor tdsc(DSC_time_trigger_setting, (const uint8_t*) &dig_time_trig, sizeof(dig_time_trig));
	addDescriptor(tdsc);
	setEventHandler(BLEWritten, on_digc_written);
	//for (byte pin=0; pin<nDigBits; pin++) 
	//	if (isReserved(pin) == false) 	// aios skips reserved pins
	//		pinIO::setMode(pin, IO_NONE);
	changed();
  }
  digCharacteristic digc (UUID16_CHR_DIGITAL); 
 #endif
  #ifdef ANAS
  void on_anac_subscribed(BLEDevice central, BLECharacteristic chr){
    Serial.print("analog subscribed: ");
	 byte datbuf[256];
	 int len = chr.readValue(datbuf, 256);
		anaCharacteristic * ana = (anaCharacteristic *) chr;  // cast to derived class
		byte ch = ana->anachan;  
		if (anaIO::setMode(ch, INPUT_ANALOG))  // activate the channel
			delay(100);
		anac[ch].updateVoltRange();
		for (int i=0;i<len;i++)
			Serial.print(datbuf[i], HEX);
		}
  // subclass BLECharacteristic 
  anaCharacteristic::anaCharacteristic() : BLEShortCharacteristic("",0){}
  anaCharacteristic::anaCharacteristic(char* bleuuid, uint8_t chan) : BLEShortCharacteristic(bleuuid, BLERead | BLENotify) 
  { 
    anachan = chan;
	 setEventHandler(BLESubscribed, on_anac_subscribed);
  }
  // add GATT descriptors for analog channel, storing their handles
  void anaCharacteristic::addDescriptors(void){
  }
  // checks GATT valid range descriptor to set analog reference voltage
	void anaCharacteristic::updateVoltRange(void) {
		ana_valid_range_t ana_valid_range;
		#ifdef BLUEFRUIT_H_
		ble_gatts_value_t value =
		{
			.len     = sizeof(ana_valid_range_t),
			.offset  = 0,
			.p_value = (uint8_t*) &ana_valid_range
		};
		// conn handle only needed for system attribute
		sd_ble_gatts_value_get(BLE_CONN_HANDLE_INVALID, valrngHandle, &value);
		#else  
		BLEDescriptor desc = getDescriptor(DSC_valid_range);
		#endif
		anaIO::setReference(anachan, ana_valid_range.Upper_inclusive_value /(anaSCALE/1000));  
		Serial.print("set VoltRng on:");Serial.print(anachan); Serial.print(" to [mV]:");
		Serial.println(ana_valid_range.Upper_inclusive_value /(anaSCALE/1000));
	}
	bool anaCharacteristic::notifyEnabled(void)
	{
		byte prop = properties();
		if (prop & BLENotify)
			return subscribed();
		return false;
	}
	
  anaCharacteristic anac[nAnaChan]; // analog
  #endif
  
bool digCharacteristic::notifyEnabled(void)
{
	byte prop = properties();
	if (prop & BLENotify)
		return subscribed();
	return false;
}
word digCharacteristic::changed(void)
{
	word changed = 0; //pinIO::produceBLEdig(digdata, LenDigBits);
	return changed;
}
void digCharacteristic::write(void)
{
	writeValue(digdata, sizeof(digdata));
}


// Configure the automation io service
void setupAIOS(BLEService& aios) {
	pinIO::createDigBits(nDigBits);
	#ifdef ANAS
	anaIO::createAnaPins(nAnaChan);
	#endif
	
	Serial.print("Creating AIOS with GATT service:");
	
  #ifdef ANAS  
	for (byte pini=0; pini<nAnaChan; pini++) {
	  anac[pini] = anaCharacteristic(UUID16_CHR_ANALOG, pini);  // chracteristic for each pin
	  ana_time_trigs[pini] = { ttINTERVAL,0, 60000U };		// 
	  ana_val_trigs[pini] = { vtDEVIATES, 1*anaSCALE }; // only notify when step>1V i.e. default setting
	}
	Serial.print(" bytes nAnaChan="); Serial.println(nAnaChan,DEC);
  #endif
 
 #ifdef BLUEFRUIT_H_
 #else 
 	aios.addCharacteristic(digc);
 #endif 
  
  //Serial.println("AIOS service been setup, starting to poll");
}

uint16_t takeAnaGatt(int chan) {
	uint16_t ana = anaIO::produceBLEana(chan, anaSCALE);
	return ana;
}

uint16_t mvolt[nAnaChan]; // stores latest analog values

// arduino keeps calling this when contacted
// check whether trigger condition is met, if so send notification otherwise just update the chracteristic value
word pollAIOS(ulong tick){
	//byte digdata[LenDigBits];
	word changed = digc.changed();  // pinIO::produceBLEdig(digdata, sizeof(digdata));
	ulong trun = tdiff(dig_time_trig.tookms, tick);
	if (digc.notifyEnabled()) {
		if ( changed || (dig_time_trig.condition==ttINTERVAL && trun>dig_time_trig.tm.interv) ){  // notification condition
			changed=1;
			digc.write();  //Value(digdata, sizeof(digdata));  //notify
			Serial.print("digital notified trun ms: "); Serial.println(trun, DEC); 
			dig_time_trig.tookms = tick;
		} else {
			//digc.write();  //Value(digdata, sizeof(digdata));  //save
		}
	} else if (changed) {  // just save to characterisric
		changed=0;
		digc.write();  //Value(digdata, sizeof(digdata));   // save
		Serial.println(" updated digdata");
	}

   #ifdef ANAS
	for (int chan = 0; chan < nAnaChan; chan++) {
		trun = tdiff(ana_time_trigs[chan].tookms, tick);
		uint16_t ana = takeAnaGatt(chan);  // anaIO::produceBLEana(chan);
		if (ana < 0xfff0) {	// valid sample
			if (anac[chan].notifyEnabled()) {
				Serial.print(" analog chan:");Serial.print(chan);
				if ((ana_val_trigs[chan].condition==vtDEVIATES && ana_val_trigs[chan].lev.val<abs(ana-mvolt[chan])) ||
					(ana_time_trigs[chan].condition==ttINTERVAL && trun>ana_time_trigs[chan].tm.interv)) {
					anac[chan].writeValue(ana);  // notify
					Serial.print(" trun ms:"); Serial.print(trun, DEC);
					Serial.print(" notified [V]:"); 
					ana_time_trigs[chan].tookms = tick;
					mvolt[chan] = ana;
					changed++;
				} else {
					anac[chan].writeValue(ana);  //save
					Serial.print(" [V]:");  
				}
				Serial.print(float(ana)/anaSCALE, DEC );
			} else if (anaIO::getMode(chan) == INPUT_ANALOG) {
				uint16_t ana = takeAnaGatt(chan);
				if (ana < 0xfff0) {	// valid sample
					anac[chan].writeValue(ana);  // saving to characteristic
				}
			}
		}
	} 
	#endif
	return changed;
}
