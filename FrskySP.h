#include "Arduino.h"

/**
 * Frsky Smart Port class
 */
class FrskySP_class {

  // Packet union (byte[8], uint64)
  union packet;

  static uint8_t CRC(uint8_t *packet);
  static void    sendData(uint8_t type, uint16_t id, int32_t val);

public:
  static void begin(uint8_t sensorId, uint8_t sensorValues);
  static void poll();
  static void setSensorData(uint8_t idx, uint16_t id, uint32_t val);
};

extern FrskySP_class FrskySP;
