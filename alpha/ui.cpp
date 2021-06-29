#include "ui.h"
#include "hal.h"
#include "power.h"
#include "radio.h"
#include "icons.h"
#include "storage.h"

// Increase as UI thread uses more things
#define THREAD_STACK_SIZE 5000

#define LONG_PRESS_DELAY 1000
#define LONG_PRESS_CLICK_INTERVAL 500
#define MIN_SLEEP_DURATION 1000

#ifdef HAL_M5STICK_C
  #define TS_LCD_HEIGHT 80
  #define TS_LCD_WIDTH 160
#endif

#define TS_LCD_BORDER 3

#define LINE_HEIGHT 16
#define FONTSIZE_1 1
#define FONTSIZE_2 2
#define FONTSIZE_3 3

_TS_UI TS_UI;

// Ctor
_TS_UI::_TS_UI():
  state_splash(
    [this]() { _TS_UI::state_splash_on_enter(); },
    nullptr,
    nullptr),
  state_datetime(
    [this]() { _TS_UI::state_datetime_on_enter(); },
    [this]() { _TS_UI::state_datetime_on(); },
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
  state_statistics(
    [this]() { _TS_UI::state_statistics_on_enter(); },
    nullptr,
    nullptr),
  state_statistics_info(
    [this]() { _TS_UI::state_statistics_info_on_enter(); },
    [this]() { _TS_UI::state_statistics_info_on(); },
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
  fsm.add_transition(&state_settings, &state_statistics, BUTTON_B, nullptr);
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
  
  fsm.add_transition(&state_statistics, &state_statistics_info, BUTTON_A, nullptr);
  fsm.add_transition(&state_statistics, &state_datetime, BUTTON_B, nullptr);
  fsm.add_transition(&state_statistics, &state_sleep, BUTTON_P, nullptr);

  fsm.add_transition(&state_statistics_info, &state_statistics, BUTTON_A, nullptr);
  fsm.add_transition(&state_statistics_info, &state_statistics, BUTTON_B, nullptr);
  fsm.add_transition(&state_statistics_info, &state_statistics, BUTTON_P, nullptr);

  fsm.add_transition(&state_sleep, &state_splash, BUTTON_A, nullptr);
  fsm.add_transition(&state_sleep, &state_splash, BUTTON_B, nullptr);
  fsm.add_transition(&state_sleep, &state_splash, BUTTON_P, nullptr);

  // Initialize default settings 
  // TODO, integrate with TS_Settings??
  brightness = 24;
  
  // Initialize private variables
  clickA = clickB = clickP = false;

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
  clear_ui();
  TS_HAL.lcd_cursor(0, 0);
  TS_HAL.lcd_drawbitmap(
    (TS_LCD_WIDTH - ICON_SIGMA_W) / 2 + 45,
    (TS_LCD_HEIGHT - ICON_SIGMA_H) / 2,
    ICON_SIGMA_W,
    ICON_SIGMA_H,
    icon_sigma
  );

  TS_HAL.lcd_setTextSize(FONTSIZE_3);
  TS_HAL.lcd_cursor(15, (TS_LCD_HEIGHT - LINE_HEIGHT * FONTSIZE_3) / 2);
  TS_HAL.lcd_printf("TRAC");
}

void _TS_UI::state_datetime_on_enter()
{
  clear_ui();
  draw_battery_icon();
}

void _TS_UI::state_datetime_on()
{
  TS_DateTime datetime;
  TS_HAL.rtc_get(datetime);

  TS_HAL.lcd_setTextSize(FONTSIZE_1);
  TS_HAL.lcd_cursor(40, (TS_LCD_HEIGHT - LINE_HEIGHT * FONTSIZE_2 - LINE_HEIGHT * FONTSIZE_1) / 2);
  TS_HAL.lcd_printf("%04d-%02d-%02d\n", datetime.year, datetime.month, datetime.day);
  TS_HAL.lcd_setTextSize(FONTSIZE_2);
  TS_HAL.lcd_cursor(27, (TS_LCD_HEIGHT - LINE_HEIGHT * FONTSIZE_2 - LINE_HEIGHT * FONTSIZE_1) / 2 + LINE_HEIGHT * FONTSIZE_1);
  TS_HAL.lcd_printf("%02d:%02d:%02d\n", datetime.hour, datetime.minute, datetime.second);
  // TS_HAL.lcd_printf("Battery: %d%%", TS_HAL.power_get_batt_level());
  // if (TS_HAL.power_is_charging())
  // {
  //   TS_HAL.lcd_printf(", Charging\n");
  // } else {
  //   TS_HAL.lcd_printf("          \n");
  // }
}

void draw_icon(const char* label, int16_t icon_width, int16_t icon_height, const uint16_t* icon_data, int8_t label_x_offset)
{
  TS_HAL.lcd_drawbitmap(
    (TS_LCD_WIDTH - icon_width) / 2,
    (TS_LCD_HEIGHT - icon_height - LINE_HEIGHT) / 2,
    icon_width,
    icon_height,
    icon_data
  );

  TS_HAL.lcd_setTextSize(FONTSIZE_1);
  TS_HAL.lcd_cursor(label_x_offset, (TS_LCD_HEIGHT + icon_height - LINE_HEIGHT) / 2);
  TS_HAL.lcd_printf(label);
}

void _TS_UI::state_settings_on_enter()
{
  clear_ui();
  draw_battery_icon();
  draw_icon("Settings", ICON_GEAR_LARGE_W, ICON_GEAR_LARGE_H, icon_gear_large, 55);
  // TS_HAL.lcd_drawbitmap(
  //   (TS_LCD_WIDTH - ICON_GEAR_LARGE_W) / 2,
  //   (TS_LCD_HEIGHT - ICON_GEAR_LARGE_H - LINE_HEIGHT) / 2,
  //   ICON_GEAR_LARGE_W,
  //   ICON_GEAR_LARGE_H,
  //   icon_gear_large
  // );

  // TS_HAL.lcd_setTextSize(FONTSIZE_1);
  // TS_HAL.lcd_cursor(55, (TS_LCD_HEIGHT + ICON_GEAR_LARGE_H - LINE_HEIGHT) / 2);
  // TS_HAL.lcd_printf("Settings");
}

void _TS_UI::state_settings_network_on_enter()
{
  clear_ui();
  draw_battery_icon();
  TS_HAL.lcd_setTextSize(FONTSIZE_1);
  TS_HAL.lcd_cursor(0, 10);
  TS_HAL.lcd_printf("Settings > Network\n");
  bool wifiConnected = TS_RADIO.wifi_is_connected();
  TS_HAL.lcd_printf("Status: %s\n", wifiConnected ? "Connected" : "Not Connected");
}

void _TS_UI::state_settings_brightness_on_enter()
{
  clear_ui();
  draw_battery_icon();
  TS_HAL.lcd_setTextSize(FONTSIZE_1);
  TS_HAL.lcd_cursor(0, 10);
  TS_HAL.lcd_printf("Settings > Brightness\n");
  char printedString[21] = " Brightness ----- ";
  printedString[brightness / 20 + 11] = '|';
  TS_HAL.lcd_printf("  %s  ", printedString);
  TS_HAL.lcd_brightness(brightness);
}

void _TS_UI::state_settings_brightness_active_on_enter()
{
  clear_ui();
  draw_battery_icon();
  TS_HAL.lcd_setTextSize(FONTSIZE_1);
  TS_HAL.lcd_cursor(0, 10);
  TS_HAL.lcd_printf("Settings > Brightness\n");
  char printedString[21] = " Brightness ----- ";
  printedString[brightness / 20 + 11] = '|';
  TS_HAL.lcd_printf(" [%s] ", printedString);
  TS_HAL.lcd_brightness(brightness);
}

void _TS_UI::state_settings_brightness_active_on()
{
  if (clickB)
  {
    TS_HAL.lcd_cursor(0, 10 + LINE_HEIGHT * FONTSIZE_1);
    char printedString[21] = " Brightness ----- ";  
    brightness %= 100;   
    brightness += 20;
    printedString[brightness / 20 + 11] = '|';
    TS_HAL.lcd_printf(" [%s] ", printedString);
    TS_HAL.lcd_brightness(brightness);
  }
}

void _TS_UI::state_settings_sleep_on_enter()
{
  clear_ui();
  draw_battery_icon();
  TS_HAL.lcd_setTextSize(FONTSIZE_1);
  TS_HAL.lcd_cursor(0, 10);
  TS_HAL.lcd_printf("Go to sleep?");
}

void _TS_UI::state_statistics_on_enter()
{
  clear_ui();
  draw_battery_icon();
  draw_icon("Statistics", ICON_STATS_W, ICON_STATS_H, icon_stats, 50);
}

void _TS_UI::state_statistics_info_on_enter()
{
  clear_ui();
  draw_battery_icon();
}

void _TS_UI::state_statistics_info_on()
{
  TS_HAL.lcd_setTextSize(FONTSIZE_1);
  TS_HAL.lcd_cursor(0, 10);
  TS_HAL.lcd_printf("Statistics > Info\n");
  TS_HAL.lcd_printf(" Used Storage : %d%%  \n", TS_Storage.usedspace_get_pct());
  TS_HAL.lcd_printf(" Exchanges    : %d   \n", TS_HAL.get_exchange_count());
  TS_HAL.lcd_printf(" Crashes      : %d   \n", TS_PersistMem.crashCount);
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

void _TS_UI::clear_ui()
{
  TS_HAL.lcd_clear();
  this->batteryIconIndex = -1;
}

void _TS_UI::draw_battery_icon()
{
  if (TS_HAL.power_is_charging())
  {
    TS_HAL.lcd_drawbitmap(
      TS_LCD_WIDTH - TS_LCD_BORDER - ICON_BATT_W,
      TS_LCD_BORDER,
      ICON_BATT_W,
      ICON_BATT_H,
      icon_batt_c
    );
  }
  else
  {
    uint8_t batt = TS_HAL.power_get_batt_level();
    int8_t batteryIconUpdateIndex = 0;
    const uint16_t *iconData;
    if (batt >= 90)
    {
      batteryIconUpdateIndex = 3;
    }
    else if (batt >= 65)
    {
      batteryIconUpdateIndex = 2;
    }
    else if (batt > 30)
    {
      batteryIconUpdateIndex = 1;
    }

    if (this->batteryIconIndex != batteryIconUpdateIndex)
    {
      this->batteryIconIndex = batteryIconUpdateIndex;
      switch(this->batteryIconIndex) {
        case 0:
          iconData = icon_batt_0;
          break;
        case 1:
          iconData = icon_batt_1;
          break;
        case 2:
          iconData = icon_batt_2;
          break;
        case 3:
          iconData = icon_batt_3;
          break;
        default:
          iconData = icon_batt_0;
      }
      TS_HAL.lcd_drawbitmap(
        TS_LCD_WIDTH - TS_LCD_BORDER - ICON_BATT_W,
        TS_LCD_BORDER,
        ICON_BATT_W,
        ICON_BATT_H,
        iconData
      );
    }
  }
}
