#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include "FrskySP.h"
#include "TermSim.h"

#ifdef _DEBUG
SoftwareSerial dbgSerial = SoftwareSerial(3,4); // Rx on pin 3 not connected
#endif

void setup()
{
  TermSim.begin();
  FrskySP.begin(0x1B,4);

#ifdef _DEBUG
  dbgSerial.begin(19200);
#endif
}

const char _sc_unknown[] PROGMEM = { "Unknown" };
const char _sc_temp[] PROGMEM = { "Temp" };
const char _sc_fuel[] PROGMEM = { "Fuel" };
const char _sc_battery[] PROGMEM = { "Battery" };
const char _sc_pump_v[] PROGMEM = { "Pump Voltage" };
const char _sc_error_st[] PROGMEM = { "Error Status" };

const char* const screen_name_lookup[] PROGMEM = {
  _sc_unknown, _sc_temp, _sc_fuel,
  _sc_battery, _sc_pump_v, _sc_error_st
};

void print_screen_type(uint8_t type)
{
  char buffer[16];

  const char* str_addr = (char*)pgm_read_word(&(screen_name_lookup[type]));
  strcpy_P(buffer, str_addr);

#ifdef _DEBUG
  dbgSerial.print(buffer);
#endif
}

// return true if error while parsing
void parse_float(const char* s, const char* end, float& value)
{
  value=0.0;
  float mul = 1.0;
  const char* c = end;
  do {
    c--;

    if( (*c >= '0') && (*c <= '9') ) {
      value += float(*c - '0') * mul;
      mul *= 10.0;
    }
    else if(*c == '.'){
      value /= mul;
      mul = 1.0;
    }
  } while(c != s);
}

#define T1_FIRST_ID               0x0400
#define RPM_FIRST_ID              0x0500
#define FUEL_FIRST_ID             0x0600
#define A3_FIRST_ID               0x0900
#define FUEL_QTY_FIRST_ID         0x0A10

void set_temp(float temp)
{
  uint32_t t = (uint32_t)temp;
  FrskySP.setSensorData(0,T1_FIRST_ID,t);
#ifdef _DEBUG
  dbgSerial.print("Temp=");
  dbgSerial.println(temp);
#endif
}

void set_rpm(float rpm)
{
  uint32_t r = (uint32_t)rpm;
  FrskySP.setSensorData(1,RPM_FIRST_ID,r);
#ifdef _DEBUG
  dbgSerial.print("RPM=");
  dbgSerial.println(rpm);
#endif
}

#if 0
void set_fuel_used(float fuel_ml)
{
  uint32_t f = (uint32_t)fuel_ml;
  FrskySP.setSensorData(2,FUEL_FIRST_ID,f);
  Serial.print("Fuel used=");
  Serial.println(fuel_ml);
}
#endif

void set_fuel_left(float fuel_ml)
{
  uint32_t f = (uint32_t)fuel_ml * 100;
  FrskySP.setSensorData(2,FUEL_QTY_FIRST_ID,f);
#ifdef _DEBUG
  dbgSerial.print("Fuel left=");
  dbgSerial.println(fuel_ml);
#endif
}

void set_battery(float vbat)
{
  uint32_t v = (uint32_t)(vbat * 100.0);
  FrskySP.setSensorData(3,A3_FIRST_ID,v);
#ifdef _DEBUG
  dbgSerial.print("VBat=");
  dbgSerial.println(vbat);
#endif
}

void loop()
{
  static uint32_t last_key_press=0;
  static uint8_t  last_screen_type = 0;
  static float    parsed_value = 0.0;

  FrskySP.poll();

  if(TermSim.getBufferSwapped()) {

    TermSim.startReading();
    TermSim.resetBufferSwapped();

    // parse screen type
    uint8_t screen_type = 0;
    uint8_t* read_buffer = TermSim.getReadBuffer();

    if(read_buffer[4] == 0x03) {
      // Temp screen found
      screen_type = 1;

      // parse temp
      parse_float((const char*)&(read_buffer[0]),
                  (const char*)&(read_buffer[4]),
                  parsed_value);
      set_temp(parsed_value);

      // parse RPM x100
      parse_float((const char*)&(read_buffer[6]),
                  (const char*)&(read_buffer[12]),
                  parsed_value);
      set_rpm(parsed_value*100.0);
    }
    else if(read_buffer[15] == 0x7) {
      // Fuel
      screen_type = 2;
      
      /* parse_float((const char*)&(read_buffer[10]), */
      /*             (const char*)&(read_buffer[15]), */
      /*             parsed_value); */
      /* set_fuel_used(parsed_value); */
      
      parse_float((const char*)&(read_buffer[26]),
                  (const char*)&(read_buffer[31]),
                  parsed_value);
      set_fuel_left(parsed_value);
    }
    else if(read_buffer[5] == 0x00 ||
            read_buffer[5] == 0x01 ||
            read_buffer[5] == 0x02) {
      // Battery
      screen_type = 3;
      parse_float((const char*)&(read_buffer[7]),
                  (const char*)&(read_buffer[12]),
                  parsed_value);
      set_battery(parsed_value);
    }
    else if(read_buffer[0] == 'M' &&
            read_buffer[1] == 'A' &&
            read_buffer[2] == 'X' &&
            read_buffer[3] == '-') {
      // Min-/Max-Pump voltage
      screen_type = 4;
#ifdef _DEBUG
      dbgSerial.println("V-Pump");
#endif
    }
    else {
      // Error Status
      screen_type = 5;
#ifdef _DEBUG
      dbgSerial.println("Error Status");
#endif
    }
    TermSim.stopReading();

  }

  if(!TermSim.keyPressed()) {

    static unsigned long last_ts = 0;
    unsigned long now_ts = millis();

    if(now_ts - last_ts > 800) {
      last_ts = now_ts;
      /* if(screen_type < 3) { */
        // press 'down'
      TermSim.pressKey(TermSim_class::DOWN_key);
      /* } */
      /* else { */
      /*   // press 'cancel' */
      /*   TermSim.pressKey(TermSim_class::CANCEL_key); */
      /* } */
    }
  }
}