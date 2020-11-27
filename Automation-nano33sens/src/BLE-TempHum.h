
// Temperature
//<InformativeText>Unit is in degrees Celsius with a resolution of 0.01 degrees Celsius</InformativeText>        <Requirement>Mandatory</Requirement>        <Format>sint16</Format>        <Unit>org.bluetooth.unit.thermodynamic_temperature.degree_celsius</Unit>        <DecimalExponent>-2</DecimalExponent>

// Humidity
//  <Format>uint16</Format>
//  <Unit>org.bluetooth.unit.percentage</Unit>
//  <DecimalExponent>-2</DecimalExponent>
/*   Gatt ESS profile
Environmental Sensing Service (ESS) Characteristics

Elevation 
Pressure ​
Temperature 
Humidity 
True Wind Speed 
True Wind Direction 
Apparent Wind Speed 
Apparent Wind Direction 
Gust Factor 
Pollen Concentration 
UV Index 
Irradiance 
Rainfall 
Wind Chill 
Heat Index 
Dew Point ​
Barometric Pressure Trend 
Magnetic Declination 
Magnetic Flux Density – 2D 
Magnetic Flux Density – 3D 
 */

#ifndef UUID16_SVR_ENV_SENSING
#define UUID16_SVR_ENV_SENSING "181A"  // "6c2fe8e1-2498-420e-bab4-81823e7b0c03"

#define UUID16_CHR_ELEVATION   "2A6C"
#define UUID16_CHR_PRESSURE    "2A6D"
#define UUID16_CHR_ALTITUDE    "2AB3"

#define UUID16_CHR_TEMPERATURE  "2A6E"  // 2A1C
#define UUID16_CHR_HUMIDITY     "2A6F"

bool setupHTS221(BLEService& service);
int pollHTS221(float & temperature, float & humidity, ulong mstick);

bool setupBARO(BLEService& service);
int pollBARO(float & pressure, ulong mstick);

extern float checkTrig(float val, float& prev);

#endif