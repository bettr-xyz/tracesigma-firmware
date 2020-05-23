#include "ui.h"
#include "hal.h"


// Increase as UI thread uses more things
#define THREAD_STACK_SIZE 5000

#define LONG_PRESS_DELAY 1000
#define LONG_PRESS_CLICK_INTERVAL 500
#define MIN_SLEEP_DURATION 2000

_TS_UI TS_UI;

// Ctor
_TS_UI::_TS_UI() {}

// Static function to call instance method
void _TS_UI::staticTask(void* parameter) {
  TS_UI.task(parameter);
}

void _TS_UI::begin() {
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

bool _TS_UI::read_button(bool reading, bool* down, unsigned long* timeout) {
  unsigned long now = millis();
  if (*down) {
    if (reading && now > *timeout) {
      *timeout += LONG_PRESS_CLICK_INTERVAL;
      return 1;
    } else if (!reading) {
      *down = 0;
    }
  } else {
    if (reading) {
      *timeout = now + LONG_PRESS_DELAY;
      *down = 1;
      return 1;
    }
  }
  return 0;
}

void _TS_UI::task(void* parameter) {
  UBaseType_t stackHighWaterMark;
  unsigned long savePowerStart = 0;

  bool btnADown = 0;
  unsigned long btnALongPressAt = 0;
  bool clickA = 0;
  int clickCountA = 0;

  bool btnBDown = 0;
  unsigned long btnBLongPressAt = 0;
  bool clickB = 0;
  int clickCountB = 0;

  // Reduce screen brightness to minimum visibility to reduce power consumption
  TS_HAL.lcd_sleep(false);
  TS_HAL.lcd_brightness(12);

  TS_HAL.lcd_cursor(40, 0);
  TS_HAL.lcd_printf("ALPHA TEST");

  while(true) {
    clickA = this->read_button(TS_HAL.btn_a_get() == TS_ButtonState::Short, &btnADown, &btnALongPressAt);
    if (!btnADown) {
      clickCountA = 0;
    }
    clickB = this->read_button(TS_HAL.btn_b_get() == TS_ButtonState::Short, &btnBDown, &btnBLongPressAt);
    if (!btnBDown) {
      clickCountB = 0;
    }

    if (clickA || clickB) {
      if (millis() - savePowerStart < MIN_SLEEP_DURATION) {
        continue;
      }
      if (savePowerStart) {
        savePowerStart = 0;
        clickCountA = 0;
        TS_HAL.lcd_sleep(false);
        TS_HAL.lcd_brightness(12);
      }

      TS_DateTime datetime;
      TS_HAL.rtc_get(datetime);

      TS_HAL.lcd_cursor(0, 15);
      // TODO: Does not work with F()
      TS_HAL.lcd_printf("Date: %04d-%02d-%02d\n", datetime.year, datetime.month, datetime.day);
      TS_HAL.lcd_printf("Time: %02d : %02d : %02d\n", datetime.hour, datetime.minute, datetime.second);

      TS_HAL.lcd_printf("Battery: %d%%  ", TS_HAL.power_get_batt_level(), NULL, NULL);
      if (TS_HAL.power_is_charging()) {
        TS_HAL.lcd_printf("Status: Charging    ");
      } else {
        TS_HAL.lcd_printf("Status: Not Charging");
      }

      clickCountA += clickA;
      clickCountB += clickB;

      TS_HAL.lcd_printf("Button A pressed: %d   \n", clickCountA);
      TS_HAL.lcd_printf("Button B pressed: %d   \n", clickCountB);

      stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
      Serial.print("UI: ");
      Serial.println(stackHighWaterMark);

      // Hold button A for 5 seconds to turn off screen
      if (clickCountA >= 10) {
        TS_HAL.lcd_sleep(true);
        savePowerStart = millis();
      }
    }

    if (TS_HAL.btn_a_get() == TS_ButtonState::Short) {
      Serial.println("Button A pressed");
    }

    if (TS_HAL.btn_b_get() == TS_ButtonState::Short) {
      Serial.println("Button B pressed");
    }

    TS_ButtonState powerButtonState = TS_HAL.btn_power_get();
    if (powerButtonState == TS_ButtonState::Short) {
      Serial.println("Power button short press");
    } else if (powerButtonState == TS_ButtonState::Long) {
      Serial.println("Power button long press");
    }

    TS_HAL.sleep(TS_SleepMode::Task, 20);
  }
}
