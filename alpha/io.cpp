#include "io.h"

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

IOButton::IOButton(uint8_t reqPin, TS_ButtonState(*f)()) : PIN(reqPin), FUNC(f)
{
  pinMode(PIN, INPUT_PULLUP);
  attachInterrupt(PIN, std::bind(&IOButton::isr,this), FALLING);
}

IOButton::~IOButton() 
{
  detachInterrupt(PIN);
}

void IOButton::isr() 
{
  portENTER_CRITICAL_ISR(&mux);
  irq = true;
  portEXIT_CRITICAL_ISR(&mux);
}

TS_ButtonState IOButton::get_state() 
{
  if (has_interrupt()) 
  {
    portENTER_CRITICAL_ISR(&mux);
    irq = false;
    portEXIT_CRITICAL_ISR(&mux);
    
    return FUNC();
  } 
  else 
  {
    return TS_ButtonState::NotPressed;
  }
}

bool IOButton::has_interrupt()
{
  return irq;
}
