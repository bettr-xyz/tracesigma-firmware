#include "io.h"

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

TS_IOButton::TS_IOButton(uint8_t reqPin, TS_ButtonState(*f)()) : PIN(reqPin), FUNC(f)
{
  pinMode(PIN, INPUT_PULLUP);
  attachInterrupt(PIN, std::bind(&TS_IOButton::isr,this), FALLING);
}

TS_IOButton::~TS_IOButton() 
{
  detachInterrupt(PIN);
}

void TS_IOButton::isr() 
{
  portENTER_CRITICAL_ISR(&mux);
  irq = true;
  portEXIT_CRITICAL_ISR(&mux);
}

TS_ButtonState TS_IOButton::get_state() 
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

bool TS_IOButton::has_interrupt()
{
  return irq;
}
