#include <Arduino.h>
#include <Wire.h>
#include "FrskySP.h"

uint8_t raw_buffer[32*2];
uint8_t* read_buffer = &raw_buffer[0];
uint8_t* write_buffer = &raw_buffer[32];
uint8_t write_ptr=0;

volatile uint8_t reading_buffer = false;
volatile uint8_t buffer_swapped = false;
volatile uint8_t i2c_busy = false;

void swap_buffers()
{
  uint8_t* tmp = read_buffer;
  read_buffer = write_buffer;
  write_buffer = tmp;
  buffer_swapped = true;
}

// Display state machine
// -> simulate the HD44780 in 4-bit mode
void dispSTM(uint8_t data, bool rs)
{
  static bool cg_ram_set = false;

  if(rs) {
    if(cg_ram_set) {
      cg_ram_set = false;
      return;
    }

    if(write_ptr < 32) {
      write_buffer[write_ptr] = data;
      write_ptr++;
    }
  }
  else {
    if(data & 0x80) {
      data &= 0x7F;
      if(data >= 0x40)
        write_ptr = data - 0x40 + 16;
      else {
        write_ptr = data;
        if(data == 0 && !reading_buffer)
          swap_buffers();
      }
    }
    else if(data & 0x40) {
      cg_ram_set = true;
    }
  }
}

// Triggered on I2C Write requests
// - Keypad: 1 byte (0xFF) -> ignore
// - Display: HD44780 commands
void onReceiveEvent(int bytes)
{
  // Ignore everything apart from
  // HD44780 4-bit commands (2 in a row)
  // bit-banged over PCF8574
  if(bytes != 6) {
    while(Wire.available())
      Wire.read();
    return;
  }

  // We only need the 3rd & 6th byte,
  // the others can be just wasted.
  Wire.read();
  Wire.read();
  
  uint8_t cmd = Wire.read();
  cmd &= 0xF0;
  
  Wire.read();
  Wire.read();

  uint8_t tmp = Wire.read();
  bool rs = tmp & 1;
  cmd |= tmp >> 4;

  dispSTM(cmd,rs);
}

#define DEFAULT_KEYS 0x5F

/*
  Leave key pressed for some cycles
  & wait for a few cycles with key released
*/
#define KEYS_TIMER 8
#define KEYS_RELEASED_CYCLES 2

volatile uint8_t keys = DEFAULT_KEYS;
volatile uint8_t keys_timer = 0;

// Triggered on I2C Read requests
// - Keypad: key state (active low)
// - Display: not used
void onRequestEvent()
{
  // - simulate key presses
  //   to cycle through status screens.
  Wire.write(keys);

  // leave key pressed until KEYS_RELEASED_CYCLES
  if(keys_timer > 0) {
    --keys_timer;
    if(keys_timer == KEYS_RELEASED_CYCLES)
      keys = DEFAULT_KEYS;
  }
}

void setup()
{
  Wire.begin(0x38);
  // Ignore the last address bit
  // to reply to 0x38 (display) & 0x39 (keypad)
  TWAMR = 1 << 1;

  Wire.onReceive(onReceiveEvent);
  Wire.onRequest(onRequestEvent);

  FrskySP.begin();
}

/* const char _sc_unknown[] PROGMEM = { "Unknown" }; */
/* const char _sc_temp[] PROGMEM = { "Temp" }; */
/* const char _sc_fuel[] PROGMEM = { "Fuel" }; */
/* const char _sc_battery[] PROGMEM = { "Battery" }; */
/* const char _sc_pump_v[] PROGMEM = { "Pump Voltage" }; */
/* const char _sc_error_st[] PROGMEM = { "Error Status" }; */

/* const char* const screen_name_lookup[] PROGMEM = { */
/*   _sc_unknown, _sc_temp, _sc_fuel, */
/*   _sc_battery, _sc_pump_v, _sc_error_st */
/* }; */

/* void print_screen_type(uint8_t type) */
/* { */
/*   char buffer[16]; */

/*   const char* str_addr = (char*)pgm_read_word(&(screen_name_lookup[type])); */
/*   strcpy_P(buffer, str_addr); */

/*   Serial.print(buffer); */
/* } */

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
//#define FUEL_QTY_FIRST_ID         0x0A10

void set_temp(float temp)
{
  uint32_t t = (uint32_t)temp;
  setSportNewData(0,T1_FIRST_ID,t);
  Serial.print("Temp=");
  Serial.println(temp);
}

void set_rpm(float rpm)
{
  uint32_t r = (uint32_t)rpm;
  setSportNewData(1,RPM_FIRST_ID,r);
  Serial.print("RPM=");
  Serial.println(rpm);
}

/* void set_fuel_used(float fuel_ml) */
/* { */
/*   uint32_t f = (uint32_t)fuel_ml; */
/*   setSportNewData(2,FUEL_FIRST_ID,f); */
/*   Serial.print("Fuel used="); */
/*   Serial.println(fuel_ml); */
/* } */

void set_fuel_left(float fuel_ml)
{
  uint32_t f = (uint32_t)fuel_ml;
  setSportNewData(2,FUEL_FIRST_ID,f);
  Serial.print("Fuel left=");
  Serial.println(fuel_ml);
}

void set_battery(float vbat)
{
  uint32_t v = (uint32_t)(vbat * 100.0);
  setSportNewData(3,A3_FIRST_ID,v);
  Serial.print("VBat=");
  Serial.println(vbat);
}

void loop()
{
  static uint32_t last_key_press=0;
  static uint32_t last_ts=0;
  static uint8_t  last_screen_type = 0;
  static float    parsed_value = 0.0;
  static uint32_t  i2c_ts = 0;

  if(i2c_busy) {
    i2c_busy = false;
    i2c_ts = micros();
  }
  else {
    if(micros() - i2c_ts > 1500)
      enableSportSensor();
  }
  
  if(buffer_swapped) {

    reading_buffer = true;
    buffer_swapped = false;
    last_ts = millis();

    // parse screen type
    uint8_t screen_type=0;

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
      Serial.println("V-Pump");
    }
    else {
      // Error Status
      screen_type = 5;
      Serial.println("Error Status");
    }
    reading_buffer = false;

    if(!keys_timer) {

      noInterrupts();
      if(screen_type < 3) {
        // press 'up'
        keys &= ~(0x8);
      }
      else {
        // press 'cancel'
        keys &= ~(0x2);
      }
      
      keys_timer = KEYS_TIMER;
      interrupts();
    }
  }

#if 0
  while(Serial.available()) {
    switch(Serial.read()) {
    case 'u':
      noInterrupts();
      keys &= ~(0x8);
      keys_timer = KEYS_TIMER;
      interrupts();
      break;
    case 'd':
      noInterrupts();
      keys &= ~(0x4);
      keys_timer = KEYS_TIMER;
      interrupts();
      break;
    case 'c':
      noInterrupts();
      keys &= ~(0x2);
      keys_timer = KEYS_TIMER;
      interrupts();
      break;
    }
  }
#endif
}