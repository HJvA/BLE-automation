/*
  Copyright (c) 2019 H.J.v.Aalderen.  All right reserved.

  This application is free software; you can redistribute it and/or
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
/*********************************************************************
 * implements AIOS	​BLE Automation IO Server
 * to let BLE AIOS client control the digital and analog IO on an 
 * Arduino/Adafruit Feather nRF52832 device with Bluetooth low energy
 ********************************************************************/
 
#include <bluefruit.h>
#include "Automation-server.h"
#include "pinIO.hpp"
#include "BLE-TempHum.h"
#include "BLE-AIOS.h"
#include "BLE-CCS811.h"
#include "BLE-bas.h"

// Custom UUID used to differentiate this device.
// Use any online UUID generator to generate a valid UUID.
// Note that the byte order is reversed ... CUSTOM_UUID
const uint8_t UUID_SVR_ENVIRON[] = {
	0x03, 0x0c, 0x7b, 0x3e, 0x82, 0x81, 0xb4, 0xba,
	0x0e, 0x42, 0x98, 0x24, 0xe1, 0xe8, 0x2f, 0x6c
	};
BLEService EnvironService; // = BLEService(BLEUuid(UUID_SVR_ENVIRON));

BLEDis bledis;    	// DIS (Device Information Service) helper class instance

// advertising when not connected
void startAdv(void)
{
	Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
	Bluefruit.Advertising.addTxPower();
	Bluefruit.Advertising.addService(aios);
	Bluefruit.Advertising.addName();
  
/* Start Advertising
 * - Enable auto advertising if disconnected
 * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
 * - Timeout for fast mode is 30 seconds
 * - Start(timeout) with timeout = 0 will advertise forever (until connected)   */
	Bluefruit.Advertising.restartOnDisconnect(true);
	Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
	Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
	Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

// which central is connecting
void connect_callback(uint16_t conn_handle)
{
	BLEConnection* connection = Bluefruit.Connection(conn_handle);
	char central_name[32] = { 0 };
	connection->getPeerName(central_name, sizeof(central_name));
	Serial.print("Connected to "); Serial.println(central_name);
	Bluefruit.autoConnLed(false);
	pinIO::setState(LED_BLUE, false);
}

/* Callback invoked when a connection is dropped
 * @param conn_handle connection where this event happens
 * @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h  */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
	(void) conn_handle;
	(void) reason;
	Serial.println("Disconnected");
	Serial.println("Advertising!");
	#if DEBUG
		Bluefruit.autoConnLed(true);  // enable blinking of blue LED
	#endif
}

void rssi_changed_callback(uint16_t conn_hdl, int8_t rssi)
{
	(void) conn_hdl;
	Serial.printf("Rssi = %d", rssi);
	Serial.println();
}

void setup()
{
	Serial.begin(115200);
	while ( !Serial ) delay(10);   // for nrf52840 with native usb

	Serial.println("Bluefruit Automation IO by: HJvA@hotmail.nl \n");

	Bluefruit.begin();
	Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
	Serial.println("Setup AIOS Service");
	setupAIOS();

	EnvironService = BLEService(BLEUuid(UUID_SVR_ENVIRON));
	EnvironService.begin(); // map all below to this service
	bool env = setupSHT31();
	env |= setupCCS811();
	setupBatteryService();
	Bluefruit.setName("AIOS fshome");

	// Set the callback handlers
	Bluefruit.Periph.setConnectCallback(connect_callback);
	Bluefruit.Periph.setDisconnectCallback(disconnect_callback);
	Bluefruit.setRssiCallback(rssi_changed_callback);

	// Configure and Start the Device Information Service
	//Serial.println("Configuring the Device Information Service");
	bledis.setManufacturer("Adafruit Industries / HJvA");
	bledis.setModel("Bluefruit Feather52");
	bledis.begin();

	// Setup the advertising packet(s)
	startAdv();
	Serial.println("\nAdvertising");
  
	pinIO::setMode(LED_RED, OUTPUT);  // pin 17
	pinIO::setMode(LED_BLUE, OUTPUT); // pin 19
	delay(2000);
}

bool starting = true;
void loop()
{
	float temp;
	float humidity;
	bool changed = false;
     
	if ( Bluefruit.connected() ) {
		if (starting) {
			Serial.println("starting");
			delay(10000);
			starting = false;
		}
		ulong mstick = millis();
		changed = false;
		changed |= pollSHT31(temp, humidity, mstick);
		changed |= pollCCS811(temp, humidity, mstick);
		changed |= pollAIOS(mstick);
		if (changed ) {  // having trigger condition
			pinIO::setState(LED_RED, true);
			uint8_t BatPerc = pollBatteryService();
			Serial.print(" Temp:");Serial.print(temp, DEC); Serial.print(" Humi:");Serial.print(humidity, DEC);
			Serial.print(" Bat%:");Serial.print(BatPerc);
		}
		Serial.println("  ====");
	} else
		starting = true;
	delay(20);
	pinIO::setState(LED_RED, false);
	delay(1000);
}
