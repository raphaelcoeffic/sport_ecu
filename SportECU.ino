#include <Arduino.h>

#include "config.h"
#include "FrskySP.h"

#define ECU_HELLO       "+++"
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

void setup()
{
  ecuSerial.begin(ECU_BIT_RATE);
  FrskySP.begin(SPORT_SENSOR_ID,6);

#ifdef USE_TEENSY
  Serial.begin(DBG_SERIAL_BIT_RATE);
  Serial.println("*** Serial Hornet ***");
#endif

  while(!ecuSerial.find((char*)ECU_HELLO));
  ecuSerial.print("OK\r");

#ifdef USE_TEENSY
  Serial.println("+++ -> OK");
#endif
}

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

#define BUFFER_SIZE 64

void loop()
{
  static unsigned char buffer[ECU_FRAME_LENGTH];
  static uint8_t frame_length = 0;

  static bool init_mode = true;
  static bool byte_stuffing = false;
  static uint8_t chk_sum = 0;

  FrskySP.poll();

  while(ecuSerial.available()){

    uint8_t data = ecuSerial.read();

    if(init_mode) {
      if(data == ECU_EOL) {
        ecuSerial.print("OK\r");
#ifdef USE_TEENSY
        Serial.println("OK");
#endif
      }
      else if(data == ECU_FRAME_START) {
        init_mode = false;
        frame_length = 0;
        chk_sum = 0;
      }
      continue;
    }
    else if(data == ECU_FRAME_START) {
      frame_length = 0;
      chk_sum = 0;
      continue;
    }

    if(data == ECU_BYTE_STUFF) {
      byte_stuffing = true;
      continue;
    }

    if(byte_stuffing) {
      chk_sum -= (data & ECU_STUFF_MASK);
      data |= ECU_STUFF_MASK;
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
  }
}