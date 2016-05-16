#ifndef _config_h_
#define _config_h_

#define SPORT_SENSOR_ID 0x1B

#define ECU_BIT_RATE 9600

#define DBG_SERIAL_BIT_RATE 57600

#if defined(__MK20DX256__) || defined(__MKL26Z64__)
#define USE_TEENSY // Teensy LC or 3.x
#endif

#endif
