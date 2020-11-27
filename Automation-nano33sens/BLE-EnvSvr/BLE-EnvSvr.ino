#include <ArduinoBLE.h>
#include "src/BLE-TempHum.h"
#include "src/pinIO/pinIO.h"
#include "src/AIOS/BLE-AIOS.h"

#define nDigBits  24

//#define ENV_SVR   "6c2fe8e1-2498-420e-bab4-81823e7b0c03"
BLEService   EnvSvr  (UUID16_SVR_ENV_SENSING);
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

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}


void setupEnvSvr(BLEService EnvSvr)
{
	Serial.begin(9600);
	while ( !Serial ) delay(10);   // for nrf52840 with native usb

	if (!BLE.begin()) {
		Serial.println("Failed to initialized BLE!");
	}
	Serial.print("nano33sens EnvSvr by: HJvA@hotmail.nl ,  freemem:");Serial.println(freeMemory());
	//BLE.debug(Serial);
	
	pinIO::createDigBits(nDigBits);
	setupHTS221(EnvSvr);
	setupBARO(EnvSvr);
	
}

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
	
	setupEnvSvr(EnvSvr);
	BLE.addService(EnvSvr);
	
	//setupANAC(aios);
	//setupDIGC(aios);
	
	//BLE.addService(aios);
	//BLE.setAdvertisedService(aios);
	BLE.setAdvertisedService(EnvSvr);
	
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
			} else {
				resetWDT();
				pinIO::setState(LEDR, true);
				ulong mstick = millis();
				changed = 0;
				changed += pollHTS221(temperat, humidity, mstick);
				changed += pollBARO(pressure, mstick);
				//changed += pollDIGC(mstick);
				//changed += pollANAC(mstick);
				if (changed) {  // having trigger condition
					Serial.println(",");
					pinIO::setState(LEDR, false);
					//uint8_t BatPerc = pollBatteryService();
					Serial.print(" Temp:");Serial.print(temperat, DEC); 
					Serial.print(" Humi:");Serial.print(humidity, DEC);
					Serial.print(" Baro:");Serial.print(pressure, DEC);
					//Serial.print(" Bat%:");Serial.print(BatPerc);
					Serial.print(" Rssi:");Serial.print(central.rssi());
				} else {
					Serial.print(".");
					//Serial.print(freeMemory()); Serial.print("");
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
