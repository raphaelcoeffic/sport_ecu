#include "Wire.h"
#include "TermSim.h"

#define DEFAULT_KEYS 0x5F

static uint8_t raw_buffer[32*2];
static uint8_t* read_buffer = &raw_buffer[0];
static uint8_t* write_buffer = &raw_buffer[32];
static uint8_t write_ptr=0;

volatile uint8_t reading_buffer = false;
volatile uint8_t buffer_swapped = false;

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

/*
  Leave key pressed for some cycles
  & wait for a few cycles with key released
*/
#define KEYS_TIMER 10
#define KEYS_RELEASED_CYCLES 4

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

void TermSim_class::begin()
{
  Wire.begin(0x38);
  // Ignore the last address bit
  // to reply to 0x38 (display) & 0x39 (keypad)
  TWAMR = 1 << 1;

  Wire.onReceive(onReceiveEvent);
  Wire.onRequest(onRequestEvent);
}

bool TermSim_class::getBufferSwapped()
{
  return buffer_swapped;
}

void TermSim_class::resetBufferSwapped()
{
  buffer_swapped = false;
}

void TermSim_class::startReading()
{
  reading_buffer = true;
}

void TermSim_class::stopReading()
{
  reading_buffer = false;
}

uint8_t* TermSim_class::getReadBuffer()
{
  return read_buffer;
}

bool TermSim_class::keyPressed()
{
  return keys_timer != 0;
}

void TermSim_class::pressKey(uint8_t key)
{
  noInterrupts();
  keys &= ~(key);
  keys_timer = KEYS_TIMER;
  interrupts();
}
