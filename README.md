# SportECU

This small project allows you to export telemetry data from a ProJET Hornet III ECU
(or any of the re-branded version among which IQ-Hammer, Behotec, etc...) to a
Frsky S.PORT compatible receiver (X-series).

Following sensors are implemented:
 - RPM
 - Temp1: EGT
 - A3: ECU battery voltage
 - A4: pump voltage
 - Fuel: qantity left in ml as reported by the ECU.
 - Temp2: hijacked to transmit the status.

## LUA script / OpenTx
If you are using a Frsky Taranis with OpenTx (2.1.x), you can use the provided LUA script
to display these values (see screenshot below) as well as the air speed (Frsky air speed sensor).
Possibly, the names of the sensors need to be changed/configured for the script to work properly.

![Screenshot LUA script](https://github.com/raphaelcoeffic/sport_ecu/blob/master/lua/screenshot.png?raw=true)

## Hardware

It has been tested with Teensy LC and Arduino Pro Mini (5V/16Mhz). However, it should
probably work with any Teensy 3.x as well or any other kind of Arduino board.

### ECU cable pinout

![ECU pinout connector](https://github.com/raphaelcoeffic/sport_ecu/blob/master/doc/ecu_connector_pinout.png?raw=true)

### Teensy

Teensy LC & 3.X have hardware support for half duplex inverted serial (as used by the S.PORT protocol),
which makes things quite simple on the S.PORT side. The TX line is directly connected to the S.PORT signal.

However, on the Hornet III ECU side, the ATmega uses 5V levels, which means that you might need to protect
the serial input, especially if you use a teensy LC (NOT 5V tolerant). This is easily done with a 3.3V Zener
diode backward against GND and a 1KOhm resistor in the ECU's TX line.

In case you plan to use a teensy 3.2, nothing special is needed here as it is 5V tolerant (not tested however).

### Arduino Pro Mini

On the pro mini, S.PORT is implemented with a special software serial S.PORT library 
([FrskySport](https://github.com/raphaelcoeffic/FrskySport)). A big thanks to Mike Bland (er9x/ersky9x) for the code!

This library is configured by default to use pin 4 on the Arduino. However, you can configure it to use pin 2 instead.
Then you just need to connect the arduino serial port with the ECU's. Do not forget to flash the arduino before soldering
the cable into the serial pins.
