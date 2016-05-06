#ifndef _TermSim_h_
#define _TermSim_h_

#include <Arduino.h>

class TermSim_class
{
public:
  enum Key {
    CANCEL_key = 0x2,
    DOWN_key = 0x4,
    UP_key = 0x8
  };
  
  static void begin();

  static bool getBufferSwapped();
  static void resetBufferSwapped();

  static void startReading();
  static void stopReading();

  static uint8_t* getReadBuffer();

  static bool keyPressed();
  static void pressKey(uint8_t key);
};

extern TermSim_class TermSim;

#endif
