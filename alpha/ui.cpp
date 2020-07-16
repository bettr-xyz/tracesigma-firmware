#include "ui.h"
#include "hal.h"
#include "power.h"
#include "radio.h"

// Increase as UI thread uses more things
#define THREAD_STACK_SIZE 5000

#define LONG_PRESS_DELAY 1000
#define LONG_PRESS_CLICK_INTERVAL 500
#define MIN_SLEEP_DURATION 1000
#define LINE_HEIGHT 16

_TS_UI TS_UI;

// Ctor
_TS_UI::_TS_UI():
  state_splash(
    [this]() { _TS_UI::state_splash_on_enter(); },
    nullptr,
    nullptr),
  state_datetime(
    [this]() { _TS_UI::state_datetime_on_enter(); },
    nullptr,
    nullptr),
  state_settings(
    [this]() { _TS_UI::state_settings_on_enter(); },
    nullptr,
    nullptr),
  state_settings_network(
    [this]() { _TS_UI::state_settings_network_on_enter(); },
    nullptr,
    nullptr),
  state_settings_brightness(
    [this]() { _TS_UI::state_settings_brightness_on_enter(); },
    nullptr,
    nullptr),
  state_settings_brightness_active(
    [this]() { _TS_UI::state_settings_brightness_active_on_enter(); },
    [this]() { _TS_UI::state_settings_brightness_active_on(); },
    nullptr),
  state_settings_sleep(
    [this]() { _TS_UI::state_settings_sleep_on_enter(); },
    nullptr,
    nullptr),
  state_sleep(
    [this]() { _TS_UI::state_sleep_on_enter(); },
    nullptr,
    [this]() { _TS_UI::state_sleep_on_exit(); }),
  fsm(&state_splash)
{

  // Setup state transitions
  fsm.add_transition(&state_splash, &state_datetime, BUTTON_A, nullptr);
  fsm.add_transition(&state_splash, &state_datetime, BUTTON_B, nullptr);
  fsm.add_transition(&state_splash, &state_sleep,    BUTTON_P, nullptr);

  // Button A does nothing
  fsm.add_transition(&state_datetime, &state_settings, BUTTON_B, nullptr);
  fsm.add_transition(&state_datetime, &state_sleep,    BUTTON_P, nullptr);

  fsm.add_transition(&state_settings, &state_settings_network, BUTTON_A, nullptr);
  fsm.add_transition(&state_settings, &state_datetime, BUTTON_B, nullptr);
  fsm.add_transition(&state_settings, &state_sleep,    BUTTON_P, nullptr);
  
  // Button A does nothing
  fsm.add_transition(&state_settings_network, &state_settings_brightness, BUTTON_B, nullptr);
  fsm.add_transition(&state_settings_network, &state_settings, BUTTON_P, nullptr);

  fsm.add_transition(&state_settings_brightness, &state_settings_brightness_active, BUTTON_A, nullptr);
  fsm.add_transition(&state_settings_brightness, &state_settings_sleep, BUTTON_B, nullptr);
  fsm.add_transition(&state_settings_brightness, &state_settings, BUTTON_P, nullptr);

  fsm.add_transition(&state_settings_brightness_active, &state_settings_brightness, BUTTON_A, nullptr);
  // Button B is used to toggle brightness values
  fsm.add_transition(&state_settings_brightness_active, &state_settings_brightness, BUTTON_P, nullptr);

  fsm.add_transition(&state_settings_sleep, &state_sleep, BUTTON_A, nullptr);
  fsm.add_transition(&state_settings_sleep, &state_settings_network, BUTTON_B, nullptr);
  fsm.add_transition(&state_settings_sleep, &state_settings, BUTTON_P, nullptr);
  
  fsm.add_transition(&state_sleep, &state_splash, BUTTON_A, nullptr);
  fsm.add_transition(&state_sleep, &state_splash, BUTTON_B, nullptr);
  fsm.add_transition(&state_sleep, &state_splash, BUTTON_P, nullptr);

  // Initialize default settings 
  // TODO, integrate with TS_Settings??
  brightness = 24;
  
  // Initialize private variables
  clickA = clickB = clickP = hasUpdate = false;

}


// Static function to call instance method
void _TS_UI::staticTask(void* parameter)
{
  TS_UI.task(parameter);
}

void _TS_UI::begin()
{
  // UI thread should run really fast
  xTaskCreatePinnedToCore(
    _TS_UI::staticTask, // thread fn
    "UITask",           // identifier
    THREAD_STACK_SIZE,  // stack size
    NULL,               // parameter
    3,                  // increased priority (main loop is running at priority 1, idle is 0)
    NULL,               // handle
    1);                 // core
}

bool _TS_UI::read_button(bool reading, button& btn)
{
  unsigned long now = millis();
  if (btn.down) {
    if (reading && now > btn.nextClick) {
      btn.nextClick += LONG_PRESS_CLICK_INTERVAL;
      return 1;
    } else if (!reading) {
      btn.down = 0;
    }
  } else {
    if (reading) {
      btn.nextClick = now + LONG_PRESS_DELAY;
      btn.down = 1;
      return 1;
    }
  }
  return 0;
}

void _TS_UI::task(void* parameter)
{
  UBaseType_t stackHighWaterMark;
  unsigned long savePowerStart = 0;

  button btnA;
  button btnB;
  button btnP;

  // Reduce screen brightness to minimum visibility to reduce power consumption
  TS_HAL.lcd_sleep(false);
  TS_HAL.lcd_brightness(brightness);

  while(true)
  {
    clickA = this->read_button(TS_HAL.btn_a_get() != TS_ButtonState::NotPressed, btnA);
    clickB = this->read_button(TS_HAL.btn_b_get() != TS_ButtonState::NotPressed, btnB);
    clickP = this->read_button(TS_HAL.btn_power_get() != TS_ButtonState::NotPressed, btnP);

    hasUpdate = clickA || clickB || clickP;
    
    if (clickA)
    {
      TS_UI.toggle_button_a();
    }

    if (clickB)
    {
      TS_UI.toggle_button_b();
    }

    if (clickP)
    {
      TS_UI.toggle_button_p();
    }

    TS_UI.update();

    //   stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    //   log_w("UI highwatermark: %d", stackHighWaterMark);

    TS_HAL.sleep(TS_SleepMode::Task, 20);
  }
}

void _TS_UI::update()
{
  fsm.run_machine();
}

void _TS_UI::toggle_button_a()
{
  fsm.trigger(BUTTON_A);
}

void _TS_UI::toggle_button_b()
{
  fsm.trigger(BUTTON_B);
}

void _TS_UI::toggle_button_p()
{
  fsm.trigger(BUTTON_P);
}


void _TS_UI::state_splash_on_enter()
{
  TS_HAL.lcd_clear();
  TS_HAL.lcd_cursor(0, 0);
  TS_HAL.lcd_printf("Splash screen");
}
void _TS_UI::state_datetime_on_enter()
{
  TS_HAL.lcd_clear();

  TS_DateTime datetime;
  TS_HAL.rtc_get(datetime);

  TS_HAL.lcd_cursor(0, 0);
  // TODO: Does not work with F()

  TS_HAL.lcd_printf("%04d-%02d-%02d ", datetime.year, datetime.month, datetime.day);
  TS_HAL.lcd_printf("%02d:%02d:%02d\n", datetime.hour, datetime.minute, datetime.second);
  TS_HAL.lcd_printf("Battery: %d%%", TS_HAL.power_get_batt_level());
  if (TS_HAL.power_is_charging())
  {
    TS_HAL.lcd_printf(", Charging");
  } else {
    TS_HAL.lcd_printf("          ");
  }
  TS_HAL.lcd_printf("\n");
}
void _TS_UI::state_settings_on_enter()
{
  TS_HAL.lcd_clear();
  TS_HAL.lcd_cursor(0, 0);
  TS_HAL.lcd_printf("Settings screen");
}

void _TS_UI::state_settings_network_on_enter()
{
  TS_HAL.lcd_clear();
  TS_HAL.lcd_cursor(0, 0);
  TS_HAL.lcd_printf("Settings > Network\n");
  bool wifiConnected = TS_RADIO.wifi_is_connected();
  TS_HAL.lcd_printf("Status: %s\n", wifiConnected ? "Connected" : "Not Connected");
}

void _TS_UI::state_settings_brightness_on_enter()
{
  TS_HAL.lcd_clear();
  TS_HAL.lcd_cursor(0, 0);
  TS_HAL.lcd_printf("Settings > Brightness\n");
  char printedString[21] = " Brightness ----- ";
  printedString[brightness / 20 + 11] = '|';
  TS_HAL.lcd_printf(printedString);
  TS_HAL.lcd_brightness(brightness);
}

void _TS_UI::state_settings_brightness_active_on_enter()
{
  TS_HAL.lcd_clear();
  TS_HAL.lcd_cursor(0, 0);
  TS_HAL.lcd_printf("Settings > Brightness\n");
  char printedString[21] = "[Brightness -----]";
  printedString[brightness / 20 + 11] = '|';
  TS_HAL.lcd_printf(printedString);
  TS_HAL.lcd_brightness(brightness);
}

void _TS_UI::state_settings_brightness_active_on()
{
  if (clickB)
  {
    TS_HAL.lcd_cursor(0, LINE_HEIGHT);
    char printedString[21] = "[Brightness -----]";      
    brightness += 20;
    brightness %= 100;
    printedString[brightness / 20 + 11] = '|';
    TS_HAL.lcd_printf(printedString);
    TS_HAL.lcd_brightness(brightness);
  }
}

void _TS_UI::state_settings_sleep_on_enter()
{
  TS_HAL.lcd_clear();
  TS_HAL.lcd_cursor(0, 0);
  TS_HAL.lcd_printf("Go to sleep?");
}

void _TS_UI::state_sleep_on_enter()
{
  TS_HAL.lcd_sleep(true);
}

void _TS_UI::state_sleep_on_exit()
{
  TS_HAL.lcd_sleep(false);
  TS_HAL.lcd_brightness(brightness);
}
