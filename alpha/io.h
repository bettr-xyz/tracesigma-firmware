#include <stdint.h>
#include <FunctionalInterrupt.h>
//#include <M5StickC.h>
#include <Arduino.h>

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
    bool handleInterrupt();
    bool hasInterrupt();

  private:
    const uint8_t PIN;
    volatile uint32_t numberKeyPresses;
    volatile bool pressed;
};


#endif
