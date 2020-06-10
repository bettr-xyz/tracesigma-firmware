#ifndef __TS_FSM__
#define __TS_FSM__

#include "src\FunctionFsm\src\FunctionFSM.h"

enum TS_PowerState
{
  LOW_POWER,
  HIGH_POWER,
};

class _TS_FSM
{
  public:
    _TS_FSM();
    
    void init();
    void update();
    void toggleFsm();

    TS_PowerState current_state;


  private:
    //fsm triggers
    enum Trigger
    {
      TOGGLE_SWITCH
    };

    //fsm state functions
    void lp_on_enter();
    void lp_on();
    void lp_on_exit();
    void lp_on_trans_hp();

    void hp_on_enter();
    void hp_on();
    void hp_on_exit();
    void hp_on_trans_lp();
    
  	FunctionState state_lp;
  	FunctionState state_hp;

	  FunctionFsm fsm;

    unsigned long last_print;
    unsigned long interval;

};

extern _TS_FSM TS_FSM;


#endif
