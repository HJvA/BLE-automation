
// Temperature
//<InformativeText>Unit is in degrees Celsius with a resolution of 0.01 degrees Celsius</InformativeText>        <Requirement>Mandatory</Requirement>        <Format>sint16</Format>        <Unit>org.bluetooth.unit.thermodynamic_temperature.degree_celsius</Unit>        <DecimalExponent>-2</DecimalExponent>

// Humidity
//  <Format>uint16</Format>
//  <Unit>org.bluetooth.unit.percentage</Unit>
//  <DecimalExponent>-2</DecimalExponent>


bool setupSHT31();
void pollSHT31(float & temp, float & humidity);
//void pollSHT31();
