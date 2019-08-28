/*
 * Peripheral : GATT server implementing automation-IO service
 * Central devices are clients. They read and write data from peripheral devices. 
 * Peripheral devices are servers. They provide data from sensors as readable characteristics, and provide read/writable characteristics to control actuators like motors, lights, and so forth.

https://www.silabs.com/community/wireless/bluetooth/knowledge-base.entry.html
https://www.arduino.cc/en/Reference/CurieBLE
https://github.com/adafruit/Adafruit_nRF52_Arduino/blob/master/libraries/Bluefruit52Lib/src/BLEUuid.h
https://www.bluetooth.com/specifications/gatt/
https://learn.adafruit.com/bluefruit-nrf52-feather-learning-guide/blecharacteristic
https://github.com/firmata/protocol
*/

#define ULONG_MAX  0xffffffff 

#define UUID16_CHR_DEW_POINT           0x2A7B

//IRRDIAANCE
//<InformativeText>Unit is in watt per square meter with a resolution of 0.1 W/m^2</InformativeText>        <Requirement>Mandatory</Requirement>   <Format>uint16</Format>       <Unit>org.bluetooth.unit.irradiance.watt_per_square_metre</Unit>        <DecimalExponent>-1</DecimalExponent>
#define UUID16_CHR_IRRADIANCE          0x2A77


#define UUID16_CHR_TEMPERATURE_MEASUREMENT  0x2A1C


// VO2 MAX
// <InformativeText> Unit is in Milliliter per kilogram per minutes with a resolution of 1 .  </InformativeText>  <Requirement>Mandatory</Requirement>        <Format>uint8</Format>        <DecimalExponent>0</DecimalExponent>
#define UUID16_CHR_VO2_MAX  0x2A96


#define DSC_client_characteristic_configuration	0x2902

/*
The Digital characteristic is an array of n 2-bit values in a bit field:
Value
Array of n 2-bit values (see below)
Table 3.2: Digital Characteristic value
The mandatory descriptor Number of Digitals (see Section 3.4) describes the number of bits that 
are available (the value of n in Table 3.2). 
The Digital characteristic contains the values of these bits in little endian order. This shall be ⌈n
4⌉
octets1 in length, where n is the number of bits defined in the Number of Digitals descriptor. The 
value of any bits beyond the number specified in the Number of Digitals descriptor is irrelevant, 
as these padding bits have no meaning. The maximum value of n is (ATT_MTU-3)*4. 
Each of the 2-bit fields has the following definition: 
• Value 0b00 defines the inactive state.
• Value 0b01 defines the active state.
• Value 0b10 defines the tri-state state (if available in the server). 
• Value 0b11 defines the unknown state. If received in a write operation the server shall not 
update corresponding output. The server may use this value in a read or a notify operation 
to indicate that for some reason it cannot report the value of this particular input.
The Notify and Indicate properties shall not be permitted simultaneously for the Digital 
characteristic.
If an Aggregate characteristic is supported as part of this service, the Notify and Indicate 
properties are excluded for the Digital characteristic.

*/


/*
Quick Overview
Here's what this article covers:

Master (or "central") devices scan for other devices. Usually, the master is the smartphone/tablet/PC.
 
Slave (or "peripheral") devices advertise and wait for connections. Usually, the slave is the BLE112/BLE113 module.
 
Client devices access remote resources over a BLE link using the GATT protocol. Usually, the master is also the client.
 
Server devices have a local database and access control methods, and provide resources to the remote client. Usually, the slave is also the server.
 
You can use read, write, notify, or indicate operations to move data between the client and the server.


Read and write operations are requested by the client and the server responds (or acknowledges).
 
Notify and indicate operations are enabled by the client but initiated by the server, providing a way to push data to the client.
 
Notifications are unacknowledged, while indications are acknowledged. Notifications are therefore faster, but less reliable
*/
