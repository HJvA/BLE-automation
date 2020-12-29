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
#define APFSZ 7

time_trig_t    ana_time_trigs[nAnaChan];
ana_val_trig_t ana_val_trigs[nAnaChan];
anaCharacteristic ** anac; // array of pointers
//anaCharacteristic * anac;  // array of analog channel
//vector<anaCharacteristic> anac;


void on_anac_subscribed(BLEDevice central, BLECharacteristic& chr, byte chan){
	Serial.print(" subscribing:");Serial.println(chr.uuid());
	byte datbuf[256];
	int len = chr.readValue(datbuf, 256);
	//pres_format_t  prfrm;
	byte ch =0;
	if (chr.hasDescriptor(DSC_characteristic_presentation_format)) {
		BLEDescriptor prfrmdsc = chr.descriptor(DSC_characteristic_presentation_format);
		//const uint8_t* AnaPresentationFormat = prfrmdsc.value();
		uint8_t AnaPresentationFormat[APFSZ];
		prfrmdsc.readValue(AnaPresentationFormat, APFSZ);
		ch = AnaPresentationFormat[5];
		for (byte b =0;b<APFSZ;b++) {
			uint8_t bt = AnaPresentationFormat[b];
			Serial.print(bt, HEX);
			}
	} else {
		int nd = chr.descriptorCount();
		Serial.print("nDescr:");Serial.println(nd);
		for (byte i=0;i<nd;i++){
			Serial.print(chr.descriptor(i).uuid());
		}
		Serial.println("no presFormDesc");
		//anaCharacteristic* ana = dynamic_cast<anaCharacteristic*>(&chr);
		anaCharacteristic* ana = static_cast<anaCharacteristic*>(&chr);
		ch = ana->anachan;
	}
		//memcpy(prfrmdsc.value(), &prfrm, sizeof(pres_format_t));
		//byte ch = prfrm.description;
		//anaCharacteristic * ana = (anaCharacteristic*)(&chr);  // cast to derived class
		//byte ch = ana->anachan;
		Serial.print(" chan:");Serial.println(chan);
		//Serial.print("handle:");Serial.println(chr.handle());
		
		if (chan<nAnaChan) {
			if (anaIO::setMode(chan, INPUT_ANALOG))  // activate the channel
				delay(100);
			anac[chan]->updateVoltRange();
		} else {
			Serial.print("bad chan:");Serial.println(chan);
		}
		
	Serial.print("readval:");
	for (int i=0;i<len;i++)
		Serial.print(datbuf[i], HEX);
}

void on_anac0_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 0);}
void on_anac1_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 1);}
void on_anac2_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 2);}
void on_anac3_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 3);}
void on_anac4_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 4);}
void on_anac5_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 5);}
void on_anac6_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 6);}
void on_anac7_subscribed(BLEDevice central, BLECharacteristic chr){ on_anac_subscribed(central, chr, 7);}


anaCharacteristic::anaCharacteristic():BLEShortCharacteristic(UUID16_CHR_ANALOG,BLERead){} // dummy

#ifdef ANAS
anaCharacteristic::anaCharacteristic(char* bleuuid, uint8_t chan) : 
	BLEShortCharacteristic(bleuuid, BLEBroadcast |  BLERead | BLENotify) { 
		uint8_t* AnaPresentationFormat { new uint8_t[APFSZ] 
		{
			DSC_data_type_uint16, // Format = 6 = "unsigned 16-bit integer"
			-4,   // Exponent : realval =  adcval * 10^Exponent 
			0x28, // Unit = 0x2728 = "voltage" (low byte)
			0x27, // ditto (high byte)
			0x02, // Namespace = 1 = "Bluetooth SIG Assigned Numbers"
			0,    // Description => channel , 0 = first (low byte)
			0x00, // ditto (high byte)
		}};
	anachan = chan;
	Serial.print("constr.anachan:");Serial.println(chan);
	ana_valid_range.Lower_inclusive_value= 0;
	ana_valid_range.Upper_inclusive_value= anaIO::maxVoltRange(chan);
	
	BLEDescriptor rngdesc(DSC_valid_range, (const uint8_t*) &ana_valid_range, sizeof(ana_valid_range_t));
	addDescriptor(rngdesc);
	//anac[ch].setPresentationFormatDescriptor(DSC_data_type_uint16, -4, UN_voltage_volt, 0, ch); 
	//pres_format_t  prfrm;
	//prfrm	= new (pres_format_t) { DSC_data_type_uint16, -4, UN_voltage_volt, 0, chan };
	AnaPresentationFormat[5]=chan;
	BLEDescriptor prfrmdesc(DSC_characteristic_presentation_format, AnaPresentationFormat, APFSZ); //sizeof(AnaPresentationFormat));
		
	//BLEDescriptor prfrmdesc(DSC_characteristic_presentation_format, (uint8_t*) prfrm, sizeof(pres_format_t) );
	//prfrmdesc.setvalue(AnaPresentationFormat, sizeof(AnaPresentationFormat));
	addDescriptor(prfrmdesc);
	Serial.print("DescCnt:");Serial.println(descriptorCount());
	//Serial.print("handle:");Serial.println(handle());
	switch (chan) {
		case 0: setEventHandler(BLESubscribed, on_anac0_subscribed); break;
		case 1: setEventHandler(BLESubscribed, on_anac1_subscribed); break;
		case 2: setEventHandler(BLESubscribed, on_anac2_subscribed); break;
		case 3: setEventHandler(BLESubscribed, on_anac3_subscribed); break;
		case 4: setEventHandler(BLESubscribed, on_anac4_subscribed); break;
		case 5: setEventHandler(BLESubscribed, on_anac5_subscribed); break;
		case 6: setEventHandler(BLESubscribed, on_anac6_subscribed); break;
		case 7: setEventHandler(BLESubscribed, on_anac7_subscribed); break;
	}
	
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
	if (hasDescriptor(DSC_valid_range)) {
		BLEDescriptor desc = descriptor(DSC_valid_range);
		desc.readValue((uint8_t*)&ana_valid_range, sizeof(ana_valid_range_t));
	} else {
		Serial.print("rng error:");Serial.println(DSC_valid_range);
	}
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
		anac[pini] = new anaCharacteristic (UUID16_CHR_ANALOG, pini);
		anaCharacteristic ana = *anac[pini];  //anaCharacteristic(UUID16_CHR_ANALOG, pini);
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
