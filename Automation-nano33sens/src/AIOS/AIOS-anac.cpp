/*************
 * https://www.geeksforgeeks.org/how-to-initialize-array-of-objects-with-parameterized-constructors-in-c/
 * 
 *************/


#include <Arduino.h>
#include <ArduinoBLE.h>
//#include <vector>
#include "../pinIO/pinIO.h"
#include "BLE-AIOS.h"

#define anaSCALE 10000	// exponent -4 in PresentationFormatDescriptor => 10000


time_trig_t    ana_time_trigs[nAnaChan];
ana_val_trig_t ana_val_trigs[nAnaChan];
anaCharacteristic ** anac; // array of pointers
//anaCharacteristic * anac;  // array of analog channel
//vector<anaCharacteristic> anac;

void on_anac_subscribed(BLEDevice central, BLECharacteristic chr){
	Serial.print("analog subscribed: chan: ");
	byte datbuf[256];
	int len = chr.readValue(datbuf, 256);
	anaCharacteristic * ana = (anaCharacteristic*)(&chr);  // cast to derived class
	byte ch = ana->anachan;
	Serial.println(ch);
	if (anaIO::setMode(ch, INPUT_ANALOG))  // activate the channel
		delay(100);
	anac[ch]->updateVoltRange();
	for (int i=0;i<len;i++)
		Serial.print(datbuf[i], HEX);
}

anaCharacteristic::anaCharacteristic():BLEShortCharacteristic(UUID16_CHR_ANALOG,BLERead){} // dummy

#ifdef ANAS
anaCharacteristic::anaCharacteristic(char* bleuuid, uint8_t chan) : BLEShortCharacteristic(bleuuid, BLEBroadcast |  BLERead | BLENotify) { 
	anachan = chan;
	Serial.print("constr.anachan:");Serial.println(chan);
	BLEDescriptor desc(DSC_valid_range, (const uint8_t*) &ana_valid_range, sizeof(ana_valid_range_t));
	addDescriptor(desc);
	setEventHandler(BLESubscribed, on_anac_subscribed);
}
#endif
bool anaCharacteristic::notifyEnabled(void)
{
	byte prop = properties();
	if (prop & BLENotify)
		return subscribed();
	return false;
}
 
// checks GATT valid range descriptor to set analog reference voltage
void anaCharacteristic::updateVoltRange(void) {
	#ifdef ANAS
	//ana_valid_range_t ana_valid_range;
	BLEDescriptor desc = descriptor(DSC_valid_range);
	desc.readValue((uint8_t*)&ana_valid_range, sizeof(ana_valid_range_t));
	//uint8_t buf[200];
	//desc.readValue(buf,sizeof(ana_valid_range_t));
	//memcpy(ana_valid_range,buf,sizeof(ana_valid_range_t));
	anaIO::setReference(anachan, ana_valid_range.Upper_inclusive_value /(anaSCALE/1000));  
	#endif
	Serial.print("set VoltRng on:");Serial.print(anachan); Serial.print(" to [mV]:");
	Serial.println(ana_valid_range.Upper_inclusive_value /(anaSCALE/1000));
}

void setupANAC(BLEService& aios)
{
	anaIO::createAnaPins(nAnaChan);
	anac = new anaCharacteristic*[nAnaChan];  // allready invokes dummy
	//anaCharacteristic** anac = new anaCharacteristic*[nAnaChan];
	
	for (byte pini=0; pini<nAnaChan; pini++) {
		#ifdef ANAS
		ana_time_trigs[pini] = { ttINTERVAL,0, 60000U };		// 
		ana_val_trigs[pini] = { vtDEVIATES, 1*anaSCALE }; // only notify when step>1V i.e. default setting
		//anac[pini] = anaCharacteristic(UUID16_CHR_ANALOG, pini);  // chracteristic for each pin
		anac[pini] = new anaCharacteristic (); //   (UUID16_CHR_ANALOG, pini);
		//anac.push_back(anaCharacteristic(UUID16_CHR_ANALOG, pini));
		anaCharacteristic ana = *anac[pini];  //anaCharacteristic(UUID16_CHR_ANALOG, pini);
		//anac[pini] = &ana;
		//anaCharacteristic ana = anaCharacteristic(UUID16_CHR_ANALOG, pini);
		//aios.addCharacteristic(ana);
		aios.addCharacteristic(ana);
		#endif
	}
	Serial.print(" bytes nAnaChan="); Serial.println(nAnaChan,DEC);
}

uint16_t takeAnaGatt(int chan) {
	uint16_t ana = anaIO::produceBLEana(chan, anaSCALE);
	return ana;
}

uint16_t mvolt[nAnaChan]; // stores latest analog values

word pollANAC(ulong tick)
{
	word changed =0;
	
	for (int chan = 0; chan < nAnaChan; chan++) {
		ulong trun = tdiff(ana_time_trigs[chan].tookms, tick);
		uint16_t ana = takeAnaGatt(chan);  // anaIO::produceBLEana(chan);
		if (ana < 0xfff0) {	// valid sample
			if (anac[chan]->notifyEnabled()) {
				Serial.print(" analog chan:");Serial.print(chan);
				if ((ana_val_trigs[chan].condition==vtDEVIATES && ana_val_trigs[chan].lev.val<abs(ana-mvolt[chan])) ||
					(ana_time_trigs[chan].condition==ttINTERVAL && trun>ana_time_trigs[chan].tm.interv)) {
					anac[chan]->writeValue(ana);  // notify
					Serial.print(" trun ms:"); Serial.print(trun, DEC);
					Serial.print(" notified [V]:"); 
					ana_time_trigs[chan].tookms = tick;
					mvolt[chan] = ana;
					changed++;
				} else {
					anac[chan]->writeValue(ana);  //save
					Serial.print(" [V]:");  
				}
				Serial.print(float(ana)/anaSCALE, DEC );
			} else if (anaIO::getMode(chan) == INPUT_ANALOG) {
				uint16_t ana = takeAnaGatt(chan);
				if (ana < 0xfff0) {	// valid sample
					anac[chan]->writeValue(ana);  // saving to characteristic
				}
			}
		}
	} 
	return changed;
}
