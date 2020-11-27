# BLE-automation
This project comprises Bluetooth GATT Automation IO server on Adafruit Feather nRF52832 device and a Pythonista Automation IO client on an IOS device and also an python3 Automation IO client on a raspberry pi using the bluepy library.

The GATT automation-IO profile defines an Bluetooth Low Energy device with digital-analog inputs and outputs, which can be controlled remotely by clients implementing the profile. This project implements it on an iPad using python in the Pythonista App. Now also a client on the Raspberry pi is provided, which is used in my fshome project.

As an extra, the Adafruit / Arduino software also has sensors implemented for temperature, humidity, eCO2, TVOC, using the I2C bus with the SHT31-D and the CCS811 breakout boards.

## Automation server

### GATT enhancements:
- uses writable Valid_Range descriptor to set analog reference voltage
- uses presentation format descriptor to tag analog characteristic with channel number (0..nChans)
- activates analog input channel when notification is requested for it
- sets digital bit to output when client writes to it

### Installation

The Adafruit device has to be programmed using the Arduino IDE as described on the Adafruit site. The Adafruit device and the breakout sensor boards can be put together using prototyping breadboards, but the doc folder in this project shows another option when a soldering iron is available.

## Automation client

### ios-AIOS-client

The Pythonista app has some very nice possibilities to build python 3 apps. Including charts and a graphical user interface, and having the cb module as an interface to Bluetooth Low Energy on the IOS platform. The doc folder shows some screenshots.

### rpi-AIOS-client

To setup BLE (i.e. bluepy) for Python on a Raspberry pi execute the following commands:  
  
```bash  
sudo apt-get install git build-essential libglib2.0-dev  
cd rpi-AIOS-client  
git clone https://github.com/IanHarvey/bluepy.git
cd bluepy  
pip3 install .
or python3 setup.py develop --user   
```

### What is next

ported to fshome project

## Usage

- connect the device to something usefull
- install the server software on it
- pick one of the clients
- extend the clients to do something usefull

## Refered products:

- The BLE-Automation-IO profile:
<https://www.bluetooth.org/docman/handlers/DownloadDoc.ashx?doc_id=304971>
- The Pythonista App:
<http://omz-software.com/pythonista>
- The Adafruit software library:
<https://github.com/adafruit/Adafruit_nRF52_Arduino>
- The Adafruit device:
<https://www.adafruit.com/product/3406>
- The bluepy package for BLE on a raspberry
<https://github.com/IanHarvey/bluepy>

