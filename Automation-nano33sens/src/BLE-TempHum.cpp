/*************************************************** 
  This is for the nano33sens Humidity & Temp Sensor
https://www.arduino.cc/en/Reference/ArduinoBLE

 ****************************************************/

#include <Arduino_HTS221.h>
#include <Arduino_LPS22HB.h> //Include library to read Pressure 
#include <ArduinoBLE.h>
#include "BLE-TempHum.h"
#include "AIOS/BLE-AIOS.h"  // trig structs


#define NOTIFYPERC 1.0
#define ROUND_2_INT(f) ((int)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

const float TEMPERATURE_CALIBRATION = -5.0;

BLEShortCharacteristic         st221 (UUID16_CHR_TEMPERATURE, BLEBroadcast | BLERead | BLENotify);
BLEUnsignedShortCharacteristic sh221 (UUID16_CHR_HUMIDITY,   BLEBroadcast | BLERead | BLENotify);
BLEShortCharacteristic         sbaro  (UUID16_CHR_PRESSURE  ,BLEBroadcast | BLERead | BLENotify);


float prevTemp;
float prevHum;
float prevPres;

time_trig_t env_time_trig;

bool setupBARO(BLEService& service) {
	if (!BARO.begin()) //Initialize Pressure sensor 
	{ 	Serial.println("Failed to initialize Pressure Sensor!"); 
		while (1);
	}
	prevPres =0;
	service.addCharacteristic(sbaro);
}

int pollBARO(float & pressure, ulong mstick) {
	pressure = BARO.readPressure();
  
	float changed = 0;
	ulong trun = tdiff(env_time_trig.tookms, mstick);
	if ( env_time_trig.condition==1 && trun>env_time_trig.tm.interv ) {  // notification condition
		changed += 10000;
		env_time_trig.tookms = mstick;
	}
	
	if (! isnan(pressure) ) {
		int16_t pres = ROUND_2_INT(pressure*100.0);
		changed += fabs(checkTrig(pressure, prevPres));
		if (sbaro.subscribed() && changed>0 ) {
			byte prop = sbaro.properties();
			if (prop & BLENotify) {
				Serial.print("pressure notif dev%="); Serial.println(changed, 1);
				sbaro.writeValue(pres); 
			}
		}else{  // not notifying
			sbaro.writeValue(pres); 
		}
	}
	return ROUND_2_INT(changed);
}

bool setupHTS221(BLEService& service) {
  Serial.print("Setup HTS221 TempHum Sensor ");
  if (!HTS.begin()) //Initialize Temperature and Humidity sensor 
  {  Serial.println("Failed to initialize HTS221!"); 
     while (1);
  }
  
  // setup default trigger conditions
  env_time_trig = (time_trig_t) { 1, millis(), 300000U };  // every 5 minutes
  
  prevTemp = 0.0;
  prevHum  = 0.0;
  /*
  st221 = BLECharacteristic(UUID16_CHR_TEMPERATURE);
  sh221 = BLECharacteristic(UUID16_CHR_HUMIDITY);
  
  st221.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  st221.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  st221.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  */
  //st221.begin();
  uint8_t meta[sizeof(env_time_trig)];
  memcpy(meta, &env_time_trig, sizeof(env_time_trig));
  BLEDescriptor ttrigset (DSC_time_trigger_setting, meta, sizeof(env_time_trig));
  st221.addDescriptor(ttrigset);
  /*
  st221.addDescriptor(DSC_time_trigger_setting, &env_time_trig, sizeof(env_time_trig), SECMODE_OPEN, SECMODE_OPEN);
  sh221.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  sh221.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  sh221.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  */
  //sh221.begin();
  service.addCharacteristic(st221);
  service.addCharacteristic(sh221);
  return true;
}

float checkTrig(float val, float & prev) {
  float chg = (val - prev) * 100.0 / prev;
  if (fabs(chg) > NOTIFYPERC) {
    Serial.print(" Prv:"); Serial.print(prev);
    prev = val;
    return chg;
  }
  return 0.0;
}

// check for trigger condition (a large change since prev notific),
// if so notify when enabled otherwise just write
int pollHTS221(float & temperature, float & humidity, ulong mstick) {
  float temp = HTS.readTemperature();
  temperature = temp + TEMPERATURE_CALIBRATION;
  
  float Hum = HTS.readHumidity();
  float dp = temp - ((100.0 - Hum) / 5.0);
  humidity = 100.0 - (5.0 * (temperature - dp));
   
  float changed = 0;
  ulong trun = tdiff(env_time_trig.tookms, mstick);
  if ( env_time_trig.condition==1 && trun>env_time_trig.tm.interv ){  // notification condition
    changed += 10000;
    env_time_trig.tookms = mstick;
  }
  
  if ( BLE.connected() ) {
    if (! isnan(temp) ) {
      int16_t tmp = ROUND_2_INT(temperature*100.0);
      changed += fabs(checkTrig(temperature, prevTemp));
      if (st221.subscribed() && changed>0 ) {
        byte prop = st221.properties();
        if (prop & BLENotify) {
          Serial.print("temperature notif dev%="); Serial.println(changed);
          st221.writeValue(tmp); 
	     }
      }else{  // not notifying
        st221.writeValue(tmp);
        //Serial.print(" temperature:"); Serial.print(temperature); 
      }
    }
    if ( !isnan(humidity) ) {
      uint16_t hum = ROUND_2_INT(humidity*100.0);
      float humchg = checkTrig(fabs(humidity-50.0)-7.0, prevHum);  // largest %dev when very dry or very wet
      changed += fabs(humchg);
      if (sh221.subscribed() && humchg!=0 ) {
        byte prop = sh221.properties();
        if (prop & BLENotify) { Serial.print("humidity notif dev%="); Serial.println(humchg); 
        sh221.writeValue(hum); }
        //Serial.print("humidity written : "); Serial.println(humidity); 
      } else {
          sh221.writeValue(hum);
          //Serial.print(" humidity:"); Serial.print(humidity); 
      }
    }
  }
  return ROUND_2_INT(changed);
}
