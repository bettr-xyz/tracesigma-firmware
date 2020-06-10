#include "ui.h"
#include "hal.h"


// Increase as UI thread uses more things
#define THREAD_STACK_SIZE 5000

#define LONG_PRESS_DELAY 1000
#define LONG_PRESS_CLICK_INTERVAL 500
#define MIN_SLEEP_DURATION 1000

_TS_UI TS_UI;

// Ctor
_TS_UI::_TS_UI() {}

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
  bool clickA = 0;

  button btnB;
  bool clickB = 0;

  button btnP;
  bool clickP = 0;

  int cursor = 0;
  int selected = -1;

  char options[][21] =
  {
    " Brightness |---- ",
    " Placeholder ",
    " Sleep",
  };

  int brightness = 24;

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
      if (selected == cursor)
      {
        selected = -1;
      } else {
        selected = cursor;
      }
    }
    if (selected == 0)
    {
        options[0][brightness / 20 + 11] = '-';
      if (clickB)
      {
        brightness %= 100;
        brightness += 20;
      }
      if (clickP)
      {
        brightness += 60;
        brightness %= 100;
        brightness += 20;
      }
      options[0][brightness / 20 + 11] = '|';
      TS_HAL.lcd_brightness(brightness);
    }
    else
    {
      if (clickB)
      {
        ++cursor %= 3;
        selected = -1;
      }
      if (clickP)
      {
        cursor += 2;
        cursor %= 3;
        selected = -1;
      }
    }

    bool hasUpdate = clickA || clickB || clickP;

    if (hasUpdate)
    {
      if (millis() - savePowerStart < MIN_SLEEP_DURATION)
      {
        continue;
      }
      if (savePowerStart)
      {
        savePowerStart = 0;
        selected = -1;
        TS_HAL.lcd_sleep(false);
        TS_HAL.lcd_brightness(brightness);
      }

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
      }
      TS_HAL.lcd_printf("\n");
      
      for (int i = 0; i < 3; i++)
      {
        if (selected == i)
        {
          TS_HAL.lcd_printf(" [%s]  \n", options[i]);
        }
        else if (cursor == i)
        {
          TS_HAL.lcd_printf("> %s   \n", options[i]);
        }
        else
        {
          TS_HAL.lcd_printf("  %s   \n", options[i]);
        }
      }

      stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
      Serial.print("UI: ");
      Serial.println(stackHighWaterMark);

      // Select option 3 to sleep.
      if (selected == 2)
      {
        TS_HAL.lcd_sleep(true);
        savePowerStart = millis();
      }
    }

    if (TS_HAL.btn_a_get() == TS_ButtonState::Short)
    {
      Serial.println("Button A pressed");
    }

    if (TS_HAL.btn_b_get() == TS_ButtonState::Short)
    {
      Serial.println("Button B pressed");
    }

    TS_ButtonState powerButtonState = TS_HAL.btn_power_get();
    if (powerButtonState == TS_ButtonState::Short)
    {
      Serial.println("Power button short press");
      TS_HAL.lcd_cursor(0, 0);
      TS_HAL.lcd_printf("Power short");
    }
    else if (powerButtonState == TS_ButtonState::Long)
    {
      Serial.println("Power button long press");
      TS_HAL.lcd_cursor(0, 0);
      TS_HAL.lcd_printf("Power long");
    }

    TS_HAL.sleep(TS_SleepMode::Task, 20);
  }
}
