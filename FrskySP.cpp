/**
 * Arduino library for Frsky Smart Port protocol.
 */

#include "Arduino.h"
#include "FrskySP.h"

#define MAX_SENSORS                8
#define SPORT_FRAME_BEGIN       0x7E
#define SPORT_BYTE_STUFF_MARKER 0x7D
#define SPORT_BYTE_STUFF_MASK   0x20

/**
 * Smart Port protocol uses 8 bytes packets.
 * 
 * Packet format (byte): tiivvvvc
 * - t: type (1 byte)
 * - i: sensor ID (2 bytes)
 * - v: value (4 bytes - int32)
 * - c: crc
 * 
 * The uint64 presentation is much easier to use for data shifting.
 */
union FrskySP_class::packet {
    //! byte[8] presentation
    uint8_t byte[8];
    //! uint64 presentation
    uint64_t uint64;
};

struct FrskySP_SensorData {
  uint16_t id;
  uint32_t val;
  bool updated;
};

static FrskySP_SensorData _sensorTable[MAX_SENSORS];
static uint8_t _sensorTableIdx = 0;
static uint8_t _sensorId = 0;
static uint8_t _sensorValues = 0;

/**
 * Init hardware serial port
 */
void FrskySP_class::begin (uint8_t sensorId, uint8_t sensorValues)
{
  _sensorId = sensorId;
  _sensorValues = sensorValues;

  Serial.begin(57600);
}

/**
 * Calculate the CRC of a packet
 * https://github.com/opentx/opentx/blob/next/radio/src/telemetry/frsky_sport.cpp
 */
uint8_t FrskySP_class::CRC (uint8_t *packet) {
    short crc = 0;
    for (int i=0; i<8; i++) {
        crc += packet[i]; //0-1FF
        crc += crc >> 8;  //0-100
        crc &= 0x00ff;
        crc += crc >> 8;  //0-0FF
        crc &= 0x00ff;
    }
    return ~crc;
}

/**
 * Sensors logical IDs and value formats are documented in FrskySP.h.
 * 
 * Packet format:
 * content   | length | remark
 * --------- | ------ | ------
 * type      | 8 bit  | always 0x10
 * sensor ID | 16 bit | sensor's logical ID
 * data      | 32 bit | preformated data
 * crc       | 8 bit  | calculated by CRC()
 */
void FrskySP_class::sendData (uint8_t type, uint16_t id, int32_t val)
{
  union packet packet;

  packet.uint64  = (uint64_t) type | (uint64_t) id << 8 | (int64_t) val << 24;
  packet.byte[7] = CRC (packet.byte);

  // disable Rx
  UCSR0B &= ~(1<<RXEN0);

  for (int i=0; i<8; i++) {

    if((packet.byte[i] == SPORT_FRAME_BEGIN) ||
       (packet.byte[i] == SPORT_BYTE_STUFF_MARKER)) {
      Serial.write(SPORT_BYTE_STUFF_MARKER);
      Serial.write(packet.byte[i] ^ SPORT_BYTE_STUFF_MASK);
    }
    else {
      Serial.write (packet.byte[i]);
    }
  }

  // re-enable Rx
  UCSR0B |= (1<<RXEN0);
}

/**
 * Polls the serial buffer for new sensor requests
 */
void FrskySP_class::poll ()
{
  while(Serial.available()) {
    if(Serial.read() == SPORT_FRAME_BEGIN) {
      
      while (!Serial.available());
      if((Serial.read() == _sensorId) &&
         _sensorValues) {

        if(!_sensorTable[_sensorTableIdx].updated) {
          _sensorTable[_sensorTableIdx].id  = 0;
          _sensorTable[_sensorTableIdx].val = 0;
        }

        FrskySP.sendData(0x10,
                         _sensorTable[_sensorTableIdx].id,
                         _sensorTable[_sensorTableIdx].val);

        _sensorTableIdx = (_sensorTableIdx + 1) % _sensorValues;
        _sensorTable[_sensorTableIdx].updated = false;
      }
    }
  }  
}

/**
 * Updates sensor data to be sent
 */
void FrskySP_class::setSensorData(uint8_t idx, uint16_t id, uint32_t val)
{
  _sensorTable[idx].id  = id;
  _sensorTable[idx].val = val;
  _sensorTable[idx].updated = true;
}
