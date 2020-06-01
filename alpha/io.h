#include <stdint.h>
#include <FunctionalInterrupt.h>
#include <Arduino.h>
#include "hal.h"

#ifndef __TS_IO__
#define __TS_IO__

enum TS_ButtonState
{
  NotPressed,
  Short,
  Long,
};

class IOButton
{
  public:
    IOButton(uint8_t reqPin);
    ~IOButton();

    void IRAM_ATTR isr();
    TS_ButtonState get_state();
    bool has_interrupt();

  private:
    const uint8_t PIN;
    volatile bool irq;
};

#endif
