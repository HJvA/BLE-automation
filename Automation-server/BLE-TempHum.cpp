/*************************************************** 
  This is an example for the SHT31-D Humidity & Temp Sensor

  Designed specifically to work with the SHT31-D sensor from Adafruit
  ----> https://www.adafruit.com/products/2857

  These sensors use I2C to communicate, 2 pins are required to  
  interface
 ****************************************************/

#include <bluefruit.h>
#include "Adafruit_SHT31.h"
#include "BLE-AIOS.h"

#define UUID16_CHR_TEMPERATURE      0x2A6E
#define UUID16_CHR_HUMIDITY         0x2A6F
#define I2Cadr 0x44   // Set to 0x45 for alternate i2c addr
#define NOTIFYPERC 1.0
#define ROUND_2_INT(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

Adafruit_SHT31* sht31; 
BLECharacteristic st31c ;  // temperature 
BLECharacteristic sh31c ;  // humidity
float prevTemp;
float prevHum;

time_trig_t env_time_trig;

bool setupSHT31() {
  Serial.print("Setup SHT31 TempHum Sensor on I2C adr:0x");Serial.println(I2Cadr, HEX);
  sht31 = new Adafruit_SHT31(); // = Adafruit_SHT31();
   
  if (! sht31->begin(I2Cadr)) {   
    Serial.println("Couldn't find SHT31");
    delete sht31;
    sht31 = NULL;
    return false;
  }
  // setup default trigger conditions
  env_time_trig = (time_trig_t) { 1, millis(), 300000U };  // every 5 minutes
  
  prevTemp = 0.0;
  prevHum = 0.0;
  st31c = BLECharacteristic(UUID16_CHR_TEMPERATURE);
  sh31c = BLECharacteristic(UUID16_CHR_HUMIDITY);
  
  st31c.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  st31c.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  st31c.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  //st31c.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  //st31c.setWriteCallback(write_callback);
  //st31c.setReadCallback(read_callback);
  st31c.begin();
  st31c.addDescriptor(DSC_time_trigger_setting, &env_time_trig, sizeof(env_time_trig), SECMODE_OPEN, SECMODE_OPEN);

  sh31c.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  sh31c.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  sh31c.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  sh31c.begin();
  
  return true;
}

float checkTrig(float val, float & prev) {
  float chg = (val - prev) * 100.0 / prev;
  if (fabs(chg) > NOTIFYPERC) {
    Serial.print(" THchange%:"); Serial.println(chg);
    prev = val;
    return chg;
  }
  return 0.0;
}

bool pollSHT31(float & temp, float & humidity, ulong mstick) {
  if (sht31 == NULL)
    return false;
  temp = sht31->readTemperature();
  humidity = sht31->readHumidity();
  float changed = 0;
  ulong trun = tdiff(env_time_trig.tookms, mstick);
  if ( env_time_trig.condition==1 && trun>env_time_trig.tm.interv ){  // notification condition
    changed += 10000;
    env_time_trig.tookms = mstick;
  }
  
  if ( Bluefruit.connected() ) {
    if (! isnan(temp) ) {
      int16_t tmp = ROUND_2_INT(temp*100.0);
      changed += fabs(checkTrig(temp, prevTemp));
      if (st31c.notifyEnabled() && changed>0.0 ) {
        if ( st31c.notify16(tmp) ){
          Serial.print("temperature notified : "); Serial.print(temp);  
        }else{
          st31c.write16(tmp);
          Serial.println("ERROR: NO Notify for temp");
        }
      }else{  // not notifying
        st31c.write16(tmp);
        Serial.print(" temperature:"); Serial.print(temp); 
      }
    }
    if (! isnan(humidity) ) {
      uint16_t hum = ROUND_2_INT(humidity*100.0);
      float humchg = checkTrig(humidity, prevHum);
      changed += fabs(humchg);
      if (sh31c.notifyEnabled() && humchg>0.0 ) {
        if ( sh31c.notify16(hum) ){
          Serial.print("humidity notified : "); Serial.print(humidity);  
        }else{
          sh31c.write16(hum);
          //Serial.print("humidity written : "); Serial.println(humidity); 
          //Serial.println("ERROR: Notify not set in the CCCD or not connected!");
        }
       }else{
          sh31c.write16(hum);
          Serial.print(" humidity:"); Serial.print(humidity); 
       }
    }
  }
  return changed;
}
