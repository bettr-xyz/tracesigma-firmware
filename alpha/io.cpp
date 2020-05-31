#include "io.h"
#include "hal.h"

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

volatile bool interruptAvailable;

IOButton::IOButton(uint8_t reqPin) : PIN(reqPin)
{
  pinMode(PIN, INPUT_PULLUP);
  attachInterrupt(PIN, std::bind(&IOButton::isr,this), FALLING);
};

IOButton::~IOButton() 
{
  detachInterrupt(PIN);
}

void IOButton::isr() 
{
  portENTER_CRITICAL_ISR(&mux);
  numberKeyPresses += 1;
  pressed = true;
  portEXIT_CRITICAL_ISR(&mux);
}

bool IOButton::handleInterrupt() 
{
  if (hasInterrupt()) 
  {
    portENTER_CRITICAL_ISR(&mux);
    pressed = false;
    portEXIT_CRITICAL_ISR(&mux);

    return true;
  } else 
  {
    return false;
  }
}

bool IOButton::hasInterrupt(){
  return pressed;
}
