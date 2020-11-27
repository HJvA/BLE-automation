/*************
 *
 *************/


#include <Arduino.h>
#include <ArduinoBLE.h>
#include "../pinIO/pinIO.h"
#include "BLE-AIOS.h"

time_trig_t dig_time_trig;
dig_val_trig_t dig_val_trig; 

void on_digc_written(BLEDevice central, BLECharacteristic chr){
	//if (chr.uuid == digc.uuid){
	Serial.print("digital received: ");
	byte datbuf[chr.valueSize()];
	int len = chr.readValue(datbuf, chr.valueLength());
	pinIO::acceptBLEdig(datbuf, len);
	for (int i=0;i<len;i++)
		Serial.print(datbuf[i], HEX);
	}
  //digCharacteristic::digCharacteristic() : BLECharacteristic("",0,""){}
digCharacteristic::digCharacteristic(char* bleuuid) : 
     BLECharacteristic(bleuuid, BLERead | BLEWrite | BLENotify, LenDigBits, true) 
{ 
	// setup default trigger conditions (at least every 5 minutes)
	dig_time_trig = (time_trig_t) { ttINTERVAL, millis(), 300000U };  // { .condition=1, .tookms=0, .interv=5s };
	
	//dig_val_trig = { 0x00 };  // { .condition=0, .mask={} };  // not used
	/*
	Serial.print(" LenDigBits="); Serial.print(LenDigBits, DEC);
	*/
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
bool digCharacteristic::notifyEnabled(void)
{
	byte prop = properties();
	if (prop & BLENotify)
		return subscribed();
	return false;
}
word digCharacteristic::changed(void)
{
	word changed = pinIO::produceBLEdig(digdata, LenDigBits);
	return changed;
}
void digCharacteristic::write(void)
{
	writeValue(digdata, sizeof(digdata));
}

digCharacteristic digc (UUID16_CHR_DIGITAL); 


void setupDIGC(BLEService& aios)
{
	pinIO::createDigBits(nDigBits);
	aios.addCharacteristic(digc);
}
word pollDIGC(ulong tick)
{
	word changed = digc.changed(); 
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
	return changed;
}
