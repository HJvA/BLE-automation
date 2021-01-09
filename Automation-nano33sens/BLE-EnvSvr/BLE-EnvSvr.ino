
#include <Arduino.h>
#include <ArduinoBLE.h>

#ifdef uRAM
#define RAMEND 0
#define SP 0
#include <MemoryUsage.h>
#endif

#include "src/BLE-TempHum.h"
#include "src/pinIO/pinIO.h"
#include "src/AIOS/BLE-AIOS.h"


//#define ENV_SVR   "6c2fe8e1-2498-420e-bab4-81823e7b0c03"
//BLEService   EnvSvr  (UUID16_SVR_ENV_SENSING);
BLEService   aios    (UUID16_SVR_AUTOMATION_IO);

void onBLEConnected(BLEDevice central) {
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  pinIO::setState(LEDB, false);
}

void onBLEDisconnected(BLEDevice central) {
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  pinIO::setState(LEDB, true);
}

void setupEnvSvr(BLEService EnvSvr)
{
	Serial.begin(9600);
	while ( !Serial ) delay(10);   // for nrf52840 with native usb

	if (!BLE.begin()) {
		Serial.println("Failed to initialized BLE!");
	}
	Serial.print("nano33sens EnvSvr by: HJvA@hotmail.nl ,  freemem:");
	//BLE.debug(Serial);
	
	//pinIO::createDigBits(nDigBits);
	setupHTS221(EnvSvr);
	setupBARO(EnvSvr);
	
}
#ifdef WDT
// WatchDogTimer
void enableWDT() {
  //Configure WDT on nRF52840.
  NRF_WDT->CONFIG         = 0x01;     // Configure WDT to run when CPU is asleep
  NRF_WDT->CRV            = 3932159;  // Timeout set to 120 seconds, timeout[s] = (CRV-1)/32768
  NRF_WDT->RREN           = 0x01;     // Enable the RR[0] reload register
  NRF_WDT->TASKS_START    = 1;        // Start WDT    
}

void resetWDT() {
  // Reload the WDTs RR[0] reload register
  NRF_WDT->RR[0] = WDT_RR_RR_Reload; 
}
#endif

void setup()
{
#ifdef WDT
	enableWDT();
	resetWDT();
#endif
	// Bluetooth LE connection handlers.
	BLE.setEventHandler(BLEConnected,    onBLEConnected);
	BLE.setEventHandler(BLEDisconnected, onBLEDisconnected);
 	
	String name;
	name = "nano BLE - EnvSvr";
	BLE.setLocalName(name.c_str());
	BLE.setDeviceName(name.c_str());
	
	setupEnvSvr(aios);
	//BLE.addService(aios);
	
	#ifdef ANAS
	setupANAC(aios);
	#endif
	setupDIGC(aios);
	
	BLE.addService(aios);
	//BLE.setAdvertisedService(aios);
	BLE.setAdvertisedService(aios);
	
	// Setup the advertising packet(s)
	Serial.println("\nAdvertising");
	
	//BLE.setAdvertisingInterval(1600); // x * 0.625 ms
	BLE.advertise();
#ifdef WDT
	resetWDT();
#endif
}

bool starting = true;
void loop()
{
	float temperat;
	float humidity;
	float pressure;
	int changed = 0;
     
	BLEDevice central = BLE.central();
	BLE.poll();
	
	if (central) {
		if (central.connected()) {
			if (starting) {
				Serial.print("Connected to central: ");
				Serial.print(central.address());
				Serial.print("  rssi: ");
				Serial.println(central.rssi());
				//delay(100);
				starting = false;
				//BLE.setAdvertisingInterval(6400); // x * 0.625 ms = 4 s   
				#ifdef uRAM
				FREERAM_PRINT;
				#endif
			} else {
				#ifdef WDT
				  resetWDT();
				#endif
				pinIO::setState(LEDR, true);
				ulong mstick = millis();
				changed = 0;
				changed += pollHTS221(temperat, humidity, mstick);
				changed += pollBARO(pressure, mstick);
				changed += pollDIGC(mstick);
				#ifdef ANAS
				changed += pollANAC(mstick);
				#endif
				if (changed) {  // having trigger condition
					Serial.println(",");
					pinIO::setState(LEDR, false);
					//uint8_t BatPerc = pollBatteryService();
					Serial.print(" Temp:");Serial.print(temperat, 2); 
					Serial.print(" Humi:");Serial.print(humidity, 1);
					Serial.print(" Baro:");Serial.print(pressure, 2);
					//Serial.print(" Bat%:");Serial.print(BatPerc);
					Serial.print(" Rssi:");Serial.print(central.rssi());
				} else {
					Serial.print(".");
					//Serial.print(freeRam()); Serial.print("");
					delay(400);
					//resetWDT();
				}
			}
		} else {
			starting = true; // after disconnect
			//resetWDT();
		}
	}
}
