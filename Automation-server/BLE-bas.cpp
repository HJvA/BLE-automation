
/*
  Copyright (c) 2019 H.J.v.Aalderen.  All right reserved.

  This library is free software; you can redistribute it and/or
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
#include <bluefruit.h>

#define ROUND_2_UINT(f) ((uint8_t)(f >= 0.0 ? (f + 0.5) : (f - 0.5)))

BLEBas blebas;          // BAS (Battery Service) helper class instance

uint32_t vbat_pin = PIN_VBAT;             // A7 for feather nRF52832, A6 for nRF52840
 
#define VBAT_MV_PER_LSB   (0.73242188F)   // 3.0V ADC range and 12-bit ADC resolution = 3000mV/4096
 
#ifdef NRF52840_XXAA    // if this is for nrf52840
#define VBAT_DIVIDER      (0.5F)               // 150K + 150K voltage divider on VBAT
#define VBAT_DIVIDER_COMP (2.0F)          // Compensation factor for the VBAT divider
#else
#define VBAT_DIVIDER      (0.71275837F)   // 2M + 0.806M voltage divider on VBAT = (2M / (0.806M + 2M))
#define VBAT_DIVIDER_COMP (1.403F)        // Compensation factor for the VBAT divider
#endif
 
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

uint8_t mvToPercent(float mvolts) {
  if(mvolts<3300)
    return 0;
 
  if(mvolts <3600) {
    mvolts -= 3300;
    return mvolts/30;
  }
 
  mvolts -= 3600;
  return 10 + (mvolts * 0.15F );  // thats mvolts /6.66666666
}


void setupBatteryService(void) { 
  // Set the analog reference to 3.0V (default = 3.6V)
  analogReference(AR_INTERNAL_3_0);
 
  // Set the resolution to 12-bit (0..4095)
  analogReadResolution(12); // Can be 8, 10, 12 or 14
  
  // Start the BLE Battery Service and set it to 100%
  Serial.println("Configuring the Battery Service");
  blebas.begin();
  blebas.write(100);
}

uint8_t pollBatteryService(void) {
  float val = analogRead(vbat_pin) * REAL_VBAT_MV_PER_LSB;  // mV
  val = mvToPercent(val);  // percentage
  blebas.write(ROUND_2_UINT(val));
  return val;
}
