#include <stdint.h>
#include <FunctionalInterrupt.h>
//#include <M5StickC.h>
#include <Arduino.h>
#include "hal.h"

#ifndef __TS_IO__
#define __TS_IO__

#define BUTTONA 37
#define BUTTONB 39

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
    volatile uint32_t numberKeyPresses;
    volatile bool irq;
};


#endif
