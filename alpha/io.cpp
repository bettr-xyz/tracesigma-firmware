#include "io.h"

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
    
    return TS_ButtonState::Short;
  } else 
  {
    return TS_ButtonState::NotPressed;
  }
}

bool IOButton::has_interrupt(){
  return irq;
}
