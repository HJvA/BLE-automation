/*************************************************** 
  This is an example for the SHT31-D Humidity & Temp Sensor

  Designed specifically to work with the SHT31-D sensor from Adafruit
  ----> https://www.adafruit.com/products/2857

  These sensors use I2C to communicate, 2 pins are required to  
  interface
 ****************************************************/

#include <bluefruit.h>
#include "Adafruit_SHT31.h"

#define UUID16_CHR_TEMPERATURE      0x2A6E
#define UUID16_CHR_HUMIDITY         0x2A6F
#define I2Cadr 0x44   // Set to 0x45 for alternate i2c addr

#define ROUND_2_INT(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

Adafruit_SHT31* sht31; 
BLECharacteristic st31c ;  // temperature 
BLECharacteristic sh31c ;  // humidity

bool setupSHT31() {
  Serial.print("Setup SHT31 TempHum Sensor on I2C adr:0x");Serial.println(I2Cadr, HEX);
  sht31 = new Adafruit_SHT31(); // = Adafruit_SHT31();
   
  if (! sht31->begin(I2Cadr)) {   
    Serial.println("Couldn't find SHT31");
    delete sht31;
    sht31 = NULL;
    return false;
  }
  st31c = BLECharacteristic(UUID16_CHR_TEMPERATURE);
  sh31c = BLECharacteristic(UUID16_CHR_HUMIDITY);
  
  st31c.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  st31c.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  st31c.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  //st31c.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  //st31c.setWriteCallback(write_callback);
  //st31c.setReadCallback(read_callback);
  st31c.begin();

  sh31c.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  sh31c.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  sh31c.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  sh31c.begin();
  return true;
}

void pollSHT31(float & temp, float & humidity) {
  if (sht31 == NULL)
    return;
  temp = sht31->readTemperature();
  humidity = sht31->readHumidity();
  if ( Bluefruit.connected() ) {
    if (st31c.notifyEnabled()) {
      if (! isnan(temp)) {
        int16_t tmp = ROUND_2_INT(temp*100.0);
        if ( st31c.notify16(tmp) ){
          Serial.print("temperature updated : "); Serial.println(temp); 
        }else{
          Serial.println("ERROR: Notify not set in the CCCD or not connected!");
        }
      }
    }
    if (sh31c.notifyEnabled()) {
      if (! isnan(humidity)) {
        uint16_t hum = ROUND_2_INT(humidity*100.0);
        if ( sh31c.notify16(hum) ){
          Serial.print("humidity updated : "); Serial.println(humidity); 
        }else{
          Serial.println("ERROR: Notify not set in the CCCD or not connected!");
        }
      }
    }
  }
}
