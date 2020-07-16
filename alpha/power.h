#ifndef __TS_POWER__
#define __TS_POWER__

#ifndef FunctionFsm
  #include "src\FunctionFsm\src\FunctionFSM.h"
#endif

enum TS_PowerState
{
  LOW_POWER,
  HIGH_POWER,
};

class _TS_POWER
{
  public:
    _TS_POWER();
    
    void init();
    void update();
    void toggle_fsm();

    TS_PowerState get_state();


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
    
    TS_PowerState current_state;

  	FunctionState state_lp;
  	FunctionState state_hp;

	  FunctionFsm fsm;

    unsigned long last_print;
    unsigned long interval;

};

extern _TS_POWER TS_POWER;


#endif
