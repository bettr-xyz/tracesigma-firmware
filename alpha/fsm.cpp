#include "fsm.h"
#include "hal.h"

_TS_FSM TS_FSM;

// Ctor
_TS_FSM::_TS_FSM():
  state_lp([this]() { _TS_FSM::lp_on_enter(); },
           [this]() { _TS_FSM::lp_on();       },
           [this]() { _TS_FSM::lp_on_exit();  }),
            
  state_hp([this]() { _TS_FSM::hp_on_enter(); },
           [this]() { _TS_FSM::hp_on();       },
           [this]() { _TS_FSM::hp_on_exit();  }),
  fsm(&state_lp)
{
  last_print = 0;
  interval = 1000;
}

//
// private fsm state functions
//

void _TS_FSM::lp_on_enter()
{ 
  Serial.println("- Enter LP -"); 
  current_state = TS_PowerState::LOW_POWER;
}

void _TS_FSM::lp_on()
{
  if (millis() - last_print > interval)
  {
    Serial.println("STATE: LOW POWER");
    last_print = millis();
  }

  // check if charging or not charging because at full capacity
  if (TS_HAL.power_is_charging() || (TS_HAL.power_get_batt_level() == 100))
  {
    toggleFsm();
  }
}
void _TS_FSM::lp_on_exit()
{ 
  Serial.println("- Exit LP -"); 
}

void _TS_FSM::lp_on_trans_hp()
{ 
  Serial.println("- Transit from LP to HP -"); 
}

void _TS_FSM::hp_on_enter() {
  Serial.println("- Enter HP -");
  current_state = TS_PowerState::HIGH_POWER;
}

void _TS_FSM::hp_on()
{
  if (millis() - last_print > interval)
  {
    Serial.println("STATE: HIGH POWER");
    last_print = millis();
  }

  // check if power is draining
  if (!TS_HAL.power_is_charging() && (TS_HAL.power_get_batt_level() < 100))
  {
    toggleFsm();
  }
}
void _TS_FSM::hp_on_exit()
{ 
  Serial.println("- Exit HP -");
}

void _TS_FSM::hp_on_trans_lp()
{ 
  Serial.println("- Transit from HP to LP -");
}

//
// public fsm state functions
//

void _TS_FSM::init()
{
  fsm.add_transition(&state_lp, &state_hp, TOGGLE_SWITCH, [this]() { lp_on_trans_hp(); } );
  fsm.add_transition(&state_hp, &state_lp, TOGGLE_SWITCH, [this]() { hp_on_trans_lp(); } );
}

void _TS_FSM::update()
{
  fsm.run_machine();
}

void _TS_FSM::toggleFsm()
{
  fsm.trigger(TOGGLE_SWITCH);
}
