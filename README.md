# Smart Water Pump System - Arduino
Automatic plant watering program for Arduino UNO R4 WiFi.

The following programs are required to manage and collect data from edge devices:
* [Smart Water Pump System - Web UI](https://github.com/AlbertYHsC/swps_web.git)
* [Smart Water Pump System - IoT Server](https://github.com/AlbertYHsC/swps_device.git)

## Features
* Recording many data types from sensor to track plant growth environment.
* Manage this device remotely from your smartphone or PC. (by browser)

## How To Use
1. Modify `SWPS_SN` and `AES_KEY` in `swps_secrets.h` and `swps_secrets.cpp` file.
2. Upload this program to the board by arduino IDE.
3. Connect the board to the IoT server using a USB cable to initialize it.
(For Details, please see [SWPS Web UI](https://github.com/AlbertYHsC/swps_web.git).)
4. Wait for the board restart and connect to WiFi.
5. Remove USB cable from the IoT Server and power the board through an additional USB adapter.

## Dependencies
* [Adafruit ADS1X15](https://github.com/adafruit/Adafruit_ADS1X15)
* [Adafruit BME280 Library](https://github.com/adafruit/Adafruit_BME280_Library)
* [Arduinojson](https://github.com/bblanchon/ArduinoJson)
* [Crypto](https://github.com/OperatorFoundation/Crypto)

## Hardware
* Arduino UNO R4 WiFi
* Adafruit ADS1115
* Adafruit BME280
* Relay control module for Arduino and a motor
* WiFi router
* Raspberry Pi (<a color="blue">Recommended</a>) or other Linux PC

### Wire Connection
![](./edge_circuits.svg)
