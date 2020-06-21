#include "power.h"
#include "hal.h"

_TS_POWER TS_POWER;

// Ctor
_TS_POWER::_TS_POWER():
  state_lp([this]() { _TS_POWER::lp_on_enter(); },
           [this]() { _TS_POWER::lp_on();       },
           [this]() { _TS_POWER::lp_on_exit();  }),
            
  state_hp([this]() { _TS_POWER::hp_on_enter(); },
           [this]() { _TS_POWER::hp_on();       },
           [this]() { _TS_POWER::hp_on_exit();  }),
  fsm(&state_lp)
{
  last_print = 0;
  interval = 1000;
}

//
// private fsm state functions
//

void _TS_POWER::lp_on_enter()
{ 
  log_i("- Enter LP -"); 
  current_state = TS_PowerState::LOW_POWER;
}

void _TS_POWER::lp_on()
{
  if (millis() - last_print > interval)
  {
    log_i("STATE: LOW POWER");
    last_print = millis();
  }

  // check if charging or not charging because at full capacity
  if (TS_HAL.power_is_charging() || (TS_HAL.power_get_batt_level() == 100))
  {
    toggle_fsm();
  }
}
void _TS_POWER::lp_on_exit()
{ 
  log_i("- Exit LP -"); 
}

void _TS_POWER::lp_on_trans_hp()
{ 
  log_i("- Transit from LP to HP -"); 
}

void _TS_POWER::hp_on_enter() {
  log_i("- Enter HP -");
  current_state = TS_PowerState::HIGH_POWER;
}

void _TS_POWER::hp_on()
{
  if (millis() - last_print > interval)
  {
    log_i("STATE: HIGH POWER");
    last_print = millis();
  }

  // check if power is draining
  if (!TS_HAL.power_is_charging() && (TS_HAL.power_get_batt_level() < 100))
  {
    toggle_fsm();
  }
}
void _TS_POWER::hp_on_exit()
{ 
  log_i("- Exit HP -");
}

void _TS_POWER::hp_on_trans_lp()
{ 
  log_i("- Transit from HP to LP -");
}

//
// public fsm state functions
//

void _TS_POWER::init()
{
  fsm.add_transition(&state_lp, &state_hp, TOGGLE_SWITCH, [this]() { lp_on_trans_hp(); } );
  fsm.add_transition(&state_hp, &state_lp, TOGGLE_SWITCH, [this]() { hp_on_trans_lp(); } );
}

void _TS_POWER::update()
{
  fsm.run_machine();
}

void _TS_POWER::toggle_fsm()
{
  fsm.trigger(TOGGLE_SWITCH);
}

TS_PowerState _TS_POWER::get_state() 
{
  return current_state;
}
