/*
  Copyright (c) 2020 H.J.v.Aalderen.  All right reserved.
  https://www.arduino.cc/en/Guide/NANO33BLESense
  */
#include <Arduino_LSM9DS1.h> //Include the library for 9-axis IMU
#include <Arduino_LPS22HB.h> //Include library to read Pressure 
#include <Arduino_HTS221.h> //Include library to read Temperature and Humidity 
#include <Arduino_APDS9960.h> //Include library for colour, proximity and gesture recognition
#ifdef I2SAUDIO
#include <ArduinoSound.h>  // https://www.arduino.cc/en/Reference/ArduinoSound
#else 
#include <PDM.h>
#endif
#include "pinIO.hpp"

const uint8_t nAnaChan = 7;  // NUM_ANALOG_INPUTS-1; //TOTAL_ANALOG_PINS;  leave out Battery channel

const uint8_t nDigBits = NUM_DIGITAL_PINS; 
// sample rate for the input
const int sampleRate = 8000;

// size of the FFT to compute
const int fftSize = 128;

// size of the spectrum output, half of FFT size
const int spectrumSize = fftSize / 2;

// frequency of whistle to detect
const int whistleFrequency = 1250;

// map whistle frequency to FFT bin
const int whistleBin = (whistleFrequency * fftSize / sampleRate);

// array to store spectrum output
int spectrum[spectrumSize];

#ifdef I2SAUDIO
FFTAnalyzer fftAnalyzer(fftSize);
#else 
short soundSampleBuffer[256];

void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read into the sample buffer
  PDM.read(soundSampleBuffer, bytesAvailable);
}

uint16_t getSoundAverage() {
  uint32_t avg = 0;
  for (int i = 0; i < sizeof(soundSampleBuffer)/sizeof(soundSampleBuffer[0]); i++) {
    avg += soundSampleBuffer[i]*soundSampleBuffer[i];
  }
  return sqrt(avg);
}
#endif
const float TEMPERATURE_CALIBRATION = -5.0;


void setup(){
  delay (10000);  // allow host to chg port to monitor after upload
  Serial.begin(9600); //Serial monitor to display all sensor values 

  if (!IMU.begin()) //Initialize IMU sensor 
  { Serial.println("Failed to initialize IMU!"); while (1);}

  if (!BARO.begin()) //Initialize Pressure sensor 
  { Serial.println("Failed to initialize Pressure Sensor!"); while (1);}

  if (!HTS.begin()) //Initialize Temperature and Humidity sensor 
  { Serial.println("Failed to initialize Temperature and Humidity Sensor!"); while (1);}

  if (!APDS.begin()) //Initialize Colour, Proximity and Gesture sensor 
  { Serial.println("Failed to initialize Colour, Proximity and Gesture Sensor!"); while (1);}
      pinIO::createDigBits(nDigBits);
  anaIO::createAnaPins(nAnaChan);
  
  pinIO::setMode(LED_RED, OUTPUT); 
  //pinIO::setMode(LED_BLUE, OUTPUT); // pin 19
  
#ifdef I2SAUDIO
  // setup the I2S audio input for the sample rate with 32-bits per sample
  if (!AudioInI2S.begin(sampleRate, 32)) {
    Serial.println("Failed to initialize I2S input!");
    while (1); // do nothing
  }

  // configure the I2S input as the input for the FFT analyzer
  if (!fftAnalyzer.input(AudioInI2S)) {
    Serial.println("Failed to set FFT analyzer input!");
    while (1); // do nothing
  }
#else
  PDM.onReceive(onPDMdata);
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM!");
  }
#endif
}

float accel_x, accel_y, accel_z;
float gyro_x, gyro_y, gyro_z;
float mag_x, mag_y, mag_z;
float Pressure;
float Temperature, Humidity;
int Proximity;

void loop()
{
  pinIO::setState(LED_RED, true);
  //Accelerometer values 
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(accel_x, accel_y, accel_z);
    Serial.print("Accelerometer = ");Serial.print(accel_x); Serial.print(", ");Serial.print(accel_y);Serial.print(", ");Serial.println(accel_z);
  }
delay (200);

  //Gyroscope values 
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(gyro_x, gyro_y, gyro_z);
    Serial.print("Gyroscope = ");Serial.print(gyro_x); Serial.print(", ");Serial.print(gyro_y);Serial.print(", ");Serial.println(gyro_z);
  }
delay (200);

  //Magnetometer values 
  if (IMU.magneticFieldAvailable()) {
    IMU.readMagneticField(mag_x, mag_y, mag_z);
    Serial.print("Magnetometer = ");Serial.print(mag_x); Serial.print(", ");Serial.print(mag_y);Serial.print(", ");Serial.println(mag_z);
  }
delay (200);

  //Read Pressure value
  Pressure = BARO.readPressure();
  Serial.print("Pressure = ");Serial.println(Pressure);
delay (200);

  //Read Temperature value
  Temperature = HTS.readTemperature();
  float temperatureCalibrated = Temperature + TEMPERATURE_CALIBRATION;
  Serial.print("Temperature = ");Serial.println(temperatureCalibrated);
delay (200);

  //Read Humidity value
  Humidity = HTS.readHumidity();
  float dp = Temperature - ((100.0 - Humidity) / 5.0);
  float humidityCalibrated = 100.0 - (5.0 * (temperatureCalibrated - dp));
  Serial.print("Humidity = ");Serial.println(humidityCalibrated);
delay (200);

  //Proximity value
  if (APDS.proximityAvailable()) {
    Proximity = APDS.readProximity();
    Serial.print("Proximity = ");Serial.println(Proximity); 
    }
delay (200);
  pinIO::setState(LED_RED, false);
  
#ifdef I2SAUDIO
  if (fftAnalyzer.available()) {
    // analysis available, read in the spectrum
    fftAnalyzer.read(spectrum, spectrumSize);

    // map the value of the whistle bin magnitude between 0 and 255
    int ledValue = map(spectrum[whistleBin], 50000, 60000, 0, 255);

    // cap the values
    if (ledValue < 0) {
      ledValue = 0;
    } else if (ledValue > 255) {
      ledValue = 255;
    }

    // set LED brightness based on whistle bin magnitude
    //analogWrite(ledPin, ledValue);
    Serial.print("whistle 1250="); Serial.println(ledValue);
 }
#else 
 uint16_t sound = getSoundAverage();
 Serial.print("sound="); Serial.println(sound);
#endif
 
  Serial.println("_____________________________________________________"); 
  delay(1000);
}
