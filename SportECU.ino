#include <Arduino.h>

#include "config.h"
#include "FrskySP.h"

// same on arduinos & teensy LC & 3.x
#define LED_PIN 13

#define ECU_INIT_TIMEOUT 2000 /* 2 seconds */
#define ECU_PLUS          '+'
#define ECU_EOL          0x0D

#define ECU_FRAME_START  0x7E
#define ECU_BYTE_STUFF   0x7D
#define ECU_STUFF_MASK   0x20

#define ECU_CHKSUM_GOOD  0x24
#define ECU_FRAME_LENGTH   40

// Serial port used to communicate
// with the Hornet III ECU
#ifdef USE_TEENSY
  // Serial port #2 is used on Teensy LC/3.x
  #define ecuSerial Serial2
#else
  // Hardware serial port is used on Arduinos
  #define ecuSerial Serial
#endif

#define T1_FIRST_ID               0x0400
#define T2_FIRST_ID               0x0410
#define RPM_FIRST_ID              0x0500
#define A3_FIRST_ID               0x0900
#define A4_FIRST_ID               0x0910
#define FUEL_QTY_FIRST_ID         0x0a10

void processEcuFrame(uint8_t* frame)
{
  uint8_t thro = frame[14];
  uint16_t rpm  = frame[8] | (frame[9] << 8);
  uint16_t egt  = frame[10] | (frame[11] << 8);
  uint8_t vbat = frame[12];
  uint8_t vpmp = frame[13];
  uint16_t fuel = frame[15] | (frame[16] << 8);
  uint16_t status = frame[19];

#ifdef USE_TEENSY
  Serial.print("th=");
  Serial.print(thro);
  Serial.print(";rpm=");
  Serial.print(rpm);
  Serial.print(";egt=");
  Serial.print(egt);
  Serial.print(";vbat=");
  Serial.print(vbat);
  Serial.print(";vpmp=");
  Serial.print(vpmp);
  Serial.print(";fuel=");
  Serial.print(fuel);
  Serial.print(";status=");
  Serial.print(status);
  Serial.println();
#endif

  FrskySP.setSensorData(0,RPM_FIRST_ID,((uint32_t)rpm)*10);
  FrskySP.setSensorData(1,T1_FIRST_ID,egt);
  FrskySP.setSensorData(2,A3_FIRST_ID,((uint32_t)vbat)*10);
  FrskySP.setSensorData(3,A4_FIRST_ID,((uint32_t)vpmp)*10);
  FrskySP.setSensorData(4,FUEL_QTY_FIRST_ID,((uint32_t)fuel)*100);
  FrskySP.setSensorData(5,T2_FIRST_ID,status);
}

unsigned long start_ts = 0;
const char OK_str[] = "OK\r";

void setup()
{
  ecuSerial.begin(ECU_BIT_RATE);
  FrskySP.begin(SPORT_SENSOR_ID,6);

#ifdef USE_TEENSY
  Serial.begin(DBG_SERIAL_BIT_RATE);
  Serial.println("*** Serial Hornet ***");
#endif

#ifdef USE_TEENSY
  Serial.println("+++ -> OK");
#endif

  pinMode(LED_PIN, OUTPUT);
  start_ts = millis();
}

void loop()
{
  enum {
    INIT_PLUS_1=0,
    INIT_PLUS_2,
    INIT_PLUS_3,
    INIT_WAIT_4_FRAME,
    INIT_RUN
  };
  static uint8_t init_mode = INIT_PLUS_1;

  static uint8_t led_blink_rate = 1; // start with slow blinking
  static unsigned long led_ts = 0;
  
  static unsigned char buffer[ECU_FRAME_LENGTH];
  static uint8_t frame_length = 0;

  static bool byte_stuffing = false;
  static uint8_t chk_sum = 0;

  // Send OK to kickstart the ECU
  // in case the boot loader was
  // still running as the hello
  // message was sent.
  //
  if( (init_mode == INIT_PLUS_1)
      && (millis() - start_ts > ECU_INIT_TIMEOUT)) {

    ecuSerial.print(OK_str);
    start_ts = millis();
  }
  
  FrskySP.poll();

  while(ecuSerial.available()){

    uint8_t data = ecuSerial.read();

    switch(init_mode) {

    case INIT_PLUS_1:
    case INIT_PLUS_2:
    case INIT_PLUS_3:

      if(data == ECU_PLUS) {
        if(init_mode++ == INIT_PLUS_3) {
          ecuSerial.print(OK_str);
          led_blink_rate = 3; // blink fast
          
#ifdef USE_TEENSY
          Serial.println("OK");
#endif
        }

        continue;
      }
      // fall through to the next case
      // in case OK has been already
      // sent without seeing '+++'

    case INIT_WAIT_4_FRAME:

      if(data == ECU_EOL) {
        ecuSerial.print(OK_str);

#ifdef USE_TEENSY
        Serial.println("OK");
#endif
        continue;
      }

      if(data != ECU_FRAME_START)
        continue;

      init_mode = INIT_RUN;
      led_blink_rate = 0; // stay on

#ifdef USE_TEENSY
      Serial.println("ECU Wireless telemetry initialized");
#endif
      // continue with 'case INIT_RUN:'

    case INIT_RUN:

      if(data == ECU_FRAME_START) {
        frame_length = 0;
        chk_sum = 0;
        continue;
      }

      if(data == ECU_BYTE_STUFF) {
        byte_stuffing = true;
        continue;
      }

      if(byte_stuffing) {
        data ^= ECU_STUFF_MASK;
        byte_stuffing = false;
      }
    
      buffer[frame_length++] = data;
      chk_sum += data;

      if(frame_length == ECU_FRAME_LENGTH) {
        if(chk_sum == ECU_CHKSUM_GOOD) {
          processEcuFrame(buffer);
        }
#ifdef USE_TEENSY
        else {
          Serial.print("## checksum error (0x");
          Serial.print(chk_sum,HEX);
          Serial.println(") ##");
        }
#endif
        frame_length = 0;
      }
      break;
    }
  }

  if(!led_blink_rate) {
    digitalWrite(LED_PIN,HIGH);
  }
  else {
    unsigned long now_ts = millis();
    
    if(now_ts - led_ts > (2000 >> led_blink_rate)) {
      // toggle LED pin
      digitalWrite(LED_PIN,!digitalRead(LED_PIN));
      led_ts = now_ts;
    }
  }
}