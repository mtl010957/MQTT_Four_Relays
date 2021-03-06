# MQTT_Four_Relays
This code switches four relays on a module connected to an ESP8266.

The relay module used was a Sainsmart 4-Channel 5V Relay Module for
PIC ARM AVR DSP Arduino MSP430 TTL Logic. The ESP8266 module was an 
Adafruit HUZZAH ESP8266 breakout. The Sainsmart relay module is designed
5V operation, but the ESP8266 outputs can only handle the 3.3V level
used to power it. The relay module does have the option to separate the
control Vcc from the relay coil supply so wire it up with the 3.3V
output from the ESP8266 supplying that Vcc. Some relay modules may
require slightly more pull-in current in this case so if your relays
don't pull in you may have to replace the pull-up resistors with a
slightly smaller value.
 
It will connect to MQTT and listens on topic relay/1/+/set where
the '+' is the relay number given by a single digit 1- 4 for 'on'
and 'off' commands. Every 60 seconds, it will publish the current
state of each of the relays on relay/1/+/state

The configuration of the WiFi connection and the MQTT server are
setup by connecting to an access point the ESP8266 creates on first
startup or when forced by holding the flash button for 5 seconds or so then releasing it.
More details on how this works are
documented in the example code for the WiFIManager library at:
https://github.com/tzapu/WiFiManager

originally based on example by
Dimitar Manovski
support@smart-republic.com
