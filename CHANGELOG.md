# Changelog BLE-automation project

If you notice that something is missing, please open an issue or submit a PR.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/).

<!--
Sections
### Added
### Changed
### Deprecated
### Removed
### Fixed
### Breaking Changes
### Developers
-->
## [1.2.0] - 2019-12-29
### Added
- Valid_Range descriptor for analog characteristic writable, and sets reference voltage
### Changed
- anaCharacteristic subclass frm BLEcharacteristic

## [1.1.2] - 2019-12-11
### Changed
- notify only on trigger conditions

## [1.1.1] - 2019-11-22
### Changed
- rpi-AIOS-client : recoverConnection when BLE connection lost
- Automation-server : time triggers for notifying temperature,humidity,TVOC,eCO2

## [1.1.0] - 2019-11-01
### Added
- rpi-AIOS-client : BLE receiver on raspberry pi using bluepy lib
### Changed
- Environment-server uses triggers for notification on Temp,Hum,CO2,VOC

## [1.0.1] - 2019-09-22
### Fixed
- not use notifications for analog channels as they can not be distinguished
- having correct Voltage scale for analog channels
### Changed
- cosmetic : displaying labels and units
- client only requests notifying if quantity is shown
### Added 
- more trigger rules for aios notifications implemented
- various BLE descriptors added to characteristics

## [1.0.0] - 2019-08-11
### Added
- first operational ble client server version
