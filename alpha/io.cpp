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

//
// Public methods
//

TS_ButtonState TS_IOButton::get_state() 
{
  if (has_interrupt()) 
  {
    resolve_irq();
  } 
  
  return poll();
}

bool TS_IOButton::has_interrupt()
{
  return irq;
}

//
// Private methods
//

void TS_IOButton::isr() 
{
  portENTER_CRITICAL_ISR(&mux);
  irq = true;
  portEXIT_CRITICAL_ISR(&mux);
}

void TS_IOButton::resolve_irq()
{
    portENTER_CRITICAL_ISR(&mux);
    irq = false;
    portEXIT_CRITICAL_ISR(&mux);
}

TS_ButtonState TS_IOButton::poll()
{
  if (FUNC) 
  {
    return FUNC();
  } 
  else 
  {
    if (digitalRead(PIN)) 
    {
      return TS_ButtonState::NotPressed;
    } 
    else 
    {
      return TS_ButtonState::Short;
    }
  }
}
