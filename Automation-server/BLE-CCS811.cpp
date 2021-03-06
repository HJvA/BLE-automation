
#include <bluefruit.h>

#define SPARKFUN 1

#ifdef SPARKFUN  /***************/
// https://github.com/sparkfun/CCS811_Air_Quality_Breakout
//Click here to get the library: http://librarymanager/All#SparkFun_CCS811
#include "SparkFunCCS811.h" 
#define CCS811_ADDR 0x5B //Default I2C Address
#define NOTIFYPERC  8.0

CCS811* ccs = new CCS811 (CCS811_ADDR);
//CCS811 ccs(CCS811_ADDR);
#else            /***************/

#include "Adafruit_CCS811.h"
Adafruit_CCS811 ccs;

#endif  
#include "BLE-AIOS.h"

//#define UUID_CHR_ECO2  = "6c2fe8e1-2498-420e-bab4-81823e7b7397"
//#define UUID_CHR_TVOC  = "6c2fe8e1-2498-420e-bab4-81823e7b7398"
#define ROUND_2_WORD(f) ((word)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))
// 
#define DSC_charac_user_descr 			0x2901

// Custom UUID used to differentiate this device.
// Use any online UUID generator to generate a valid UUID.
// Note that the byte order is reversed ... CUSTOM_UUID
const uint8_t UUID_CHR_ECO2[] =
{
    0x97, 0x73, 0x7b, 0x3e, 0x82, 0x81, 0xb4, 0xba,
    0x0e, 0x42, 0x98, 0x24, 0xe1, 0xe8, 0x2f, 0x6c
};
const uint8_t UUID_CHR_TVOC[] =
{
    0x98, 0x73, 0x7b, 0x3e, 0x82, 0x81, 0xb4, 0xba,
    0x0e, 0x42, 0x98, 0x24, 0xe1, 0xe8, 0x2f, 0x6c
};



BLECharacteristic eCO2c ;  // equivalent CO2 
BLECharacteristic TVOCc ;  // volatile organic compounds
const char * CO2descr = "eCO2";
const char * TVOCdescr = "TVOC";
uint16_t prevCO2 = 0;
uint16_t prevVOC = 0;

time_trig_t ccs_time_trig;

float checkTrig(uint16_t val, uint16_t & prev) {
  float chg = (float(val) - float(prev)) * 100.0 / prev;
  if (fabs(chg) > NOTIFYPERC) {
    Serial.print(" CCSchng%:"); Serial.println(chg);
    prev = val;
    return chg;
  }
  return 0.0;
}

bool setupCCS811(){
  Serial.print("Setup CCS811 Sensor on I2C adr:0x");Serial.println(CCS811_ADDR, HEX);
#ifdef SPARKFUN
  CCS811Core::status returnCode = ccs->begin();
  if (returnCode != CCS811Core::SENSOR_SUCCESS){
    delete ccs;
    ccs = NULL;
    return false;
  }
#else
  if(!ccs.begin())
  {
    Serial.println("Failed to start CCS811 sensor! Please check your wiring.");
    //delete css;
    //css = NULL;
    return false;
  }
#endif

  eCO2c = BLECharacteristic(BLEUuid(UUID_CHR_ECO2));
  TVOCc = BLECharacteristic(BLEUuid(UUID_CHR_TVOC));
  
  eCO2c.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  eCO2c.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  eCO2c.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  eCO2c.begin();
  eCO2c.addDescriptor(DSC_time_trigger_setting, &ccs_time_trig, sizeof(ccs_time_trig), SECMODE_OPEN, SECMODE_OPEN);
  // setup default trigger conditions
  ccs_time_trig = (time_trig_t) { 1, millis(), 300000U };  // every 5 minutes at least

  TVOCc.setUserDescriptor(CO2descr);
  // getting errors when activating the user descr ??
  //eCO2c.addDescriptor(DSC_charac_user_descr, CO2descr, 4, SECMODE_OPEN, SECMODE_NO_ACCESS);

  TVOCc.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
  TVOCc.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  TVOCc.setFixedLen(2); // Alternatively .setMaxLen(uint16_t len);
  TVOCc.begin();
  //err_t BLECharacteristic::addDescriptor(BLEUuid bleuuid, void const * content, uint16_t len, BleSecurityMode read_perm, BleSecurityMode write_perm)
  //TVOCc.addDescriptor(DSC_charac_user_descr, TVOCdescr, strlen(TVOCdescr), SECMODE_OPEN, SECMODE_NO_ACCESS);
  TVOCc.setUserDescriptor(TVOCdescr);
  //void BLECharacteristic::setReportRefDescriptor(uint8_t id, uint8_t type)
  return true;
}

bool pollCCS811(float temp, float hum, ulong mstick){
  float changed = 0;
#ifdef SPARKFUN
  if (ccs == NULL)
     return 0;
  if (ccs->dataAvailable()) {
      if (temp>0 && hum>0 && !isnan(temp) && !isnan(hum))
          ccs->setEnvironmentalData(temp, hum);
        ccs->readAlgorithmResults(); //Calling this function updates the global tVOC and eCO2 variables
        uint16_t eCO2 = ccs->getCO2();
        uint16_t TVOC = ccs->getTVOC();
#else
      if(!ccs.readData()){
        ccs.setTempOffset(temp - 25.0);
        uint16_t eCO2 = ccs.geteCO2();
        uint16_t TVOC = ccs.getTVOC();
#endif
        ulong trun = tdiff(ccs_time_trig.tookms, mstick);
        if ( ccs_time_trig.condition==1 && trun>ccs_time_trig.tm.interv ){  // notification condition
          changed += 10000;
          ccs_time_trig.tookms = mstick;
        }
        Serial.print("eCO2:"); Serial.print(eCO2, DEC);  Serial.print(" TVOC:"); Serial.print(TVOC, DEC);
        if ( Bluefruit.connected() ) {
          changed += checkTrig(eCO2-200, prevCO2);
          if (eCO2c.notifyEnabled() && changed ) {
            eCO2c.notify16(eCO2);
            Serial.println("notifying CO2");
          } else
            eCO2c.write16(eCO2);
          changed += checkTrig(TVOC+30, prevVOC);
          if (TVOCc.notifyEnabled() && changed ) {
            TVOCc.notify16(TVOC);
            Serial.println("notifying VOC");
          } else
            TVOCc.write16(TVOC);
        }
      }
    return changed != 0.0;
}
