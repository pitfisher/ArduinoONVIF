Arduino ONVIF
Library that is supposed to implement ONVIF interaction with according equipment using Arduino platform
For now there is working ContiniousMove function implementation, but it requires manual set of ProfileToken. Also device IP must be set manually.

In order to implement more automatic workflow library is planned to be ported to ESP32 platform.

Used Libraries:
* [base64_arduino by Densaugeo, available from Arduino IDE libraries manager](https://github.com/Densaugeo/base64_arduino "Github page")
* [Cryptosuite, fork from Adafruit, should be git-cloned to your local Arduino/libraries/ folder](https://github.com/adafruit/Cryptosuite "Github page")
* [Arduino NTPClient, available from Arduino IDE libraries manager](https://github.com/arduino-libraries/NTPClient "Github page")
