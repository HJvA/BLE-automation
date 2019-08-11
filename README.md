# BLE-automation
This project comprises Bluetooth GATT Automation IO server on Adafruit Feather nRF52832 device and a Pythonista Automation IO client on an IOS device.

The GATT automation-IO profile defines an Bluetooth Low Energy device with digital-analog inputs and outputs, which can be controlled remotely by clients implementing the profile. This project implements it on an iPad using python in the Pythonista App.

As an extra, the Adafruit / Arduino software also has sensors implemented for temperature, humidity, eCO2, TVOC, using the I2C bus with the SHT31-D and the CCS811 breakout boards.

## Automation server

### Installation

The Adafruit device has to be programmed using the Arduino IDE as described on the Adafruit site. The Adafruit device and the breakout sensor boards can be put together using prototyping breadboards, but the doc folder in this project shows another option when a soldering iron is available.

## Automation client

The Pythonista app has some very nice possibilities to build python 3 apps. Including charts and a graphical user interface, and having the cb module as an interface to Bluetooth Low Energy on the IOS platform. The doc folder shows some screenshots.

I am planning to include this device in my other github project: fshome

## Usage


## Refered products:

- The BLE-Automation-IO profile:
<https://www.bluetooth.org/docman/handlers/DownloadDoc.ashx?doc_id=304971>
- The Pythonista App:
<http://omz-software.com/pythonista>
- The Adafruit software library:
<https://github.com/adafruit/Adafruit_nRF52_Arduino>
- The Adafruit device:
<https://www.adafruit.com/product/3406>



