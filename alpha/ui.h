#ifndef __TS_UI__
#define __TS_UI__

#include "src\FunctionFsm\src\FunctionFSM.h"

enum TS_UIState
{
  SPLASH,
  DATETIME,
  SETTINGS,
  SETTINGS_NETWORK,
  SETTINGS_BRIGHTNESS,
  SETTINGS_BRIGHTNESS_ACTIVE,
  SETTINGS_SLEEP,
  STATISTICS,
  STATISTICS_INFO,
  SLEEP
};

struct button {
  bool down;
  unsigned long nextClick;
};

class _TS_UI
{
  public:
    _TS_UI();
    void begin();
    void update();
    void toggle_button_a();
    void toggle_button_b();
    void toggle_button_p();
    static void staticTask(void*);


  private:
    void task(void*);
    bool read_button(bool, button&);

    int8_t batteryIconIndex;

    int brightness;

    bool clickA, clickB, clickP;

    //fsm triggers
    enum Trigger
    {
      BUTTON_A,
      BUTTON_B,
      BUTTON_P
    };

    FunctionFsm fsm;

    TS_UIState current_state;

    FunctionState state_splash;
    FunctionState state_datetime;
    FunctionState state_settings;
    FunctionState state_settings_network;
    FunctionState state_settings_brightness;
    FunctionState state_settings_brightness_active;
    FunctionState state_settings_sleep;
    FunctionState state_statistics;
    FunctionState state_statistics_info;
    FunctionState state_sleep;

    //fsm state functions
    void state_splash_on_enter();
    void state_datetime_on_enter();
    void state_datetime_on();
    void state_settings_on_enter();
    void state_settings_network_on_enter();
    void state_settings_brightness_on_enter();
    void state_settings_brightness_active_on_enter();
    void state_settings_brightness_active_on();
    void state_settings_sleep_on_enter();
    void state_statistics_on_enter();
    void state_statistics_info_on_enter();
    void state_statistics_info_on();
    void state_sleep_on_enter();
    void state_sleep_on_exit();

    void clear_ui();
    void draw_gear_icon();
    void draw_battery_icon();

};

extern _TS_UI TS_UI;


#endif
