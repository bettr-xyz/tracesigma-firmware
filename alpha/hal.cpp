#include "hal.h"
#include "esp_pm.h"

// Lower level library include decisions go here

#ifdef HAL_M5STICK_C
#include <M5StickC.h>

#elif HAL_M5STACK
#include <M5Stack.h>

#endif

#define ENTER_CRITICAL  xSemaphoreTake(halMutex, portMAX_DELAY)
#define EXIT_CRITICAL   xSemaphoreGive(halMutex)
static SemaphoreHandle_t halMutex;

// Your one and only
_TS_HAL TS_HAL;
_TS_HAL::_TS_HAL() {}

void _TS_HAL::begin()
{
  log_init();
  this->bleInitialized = false;
  halMutex = xSemaphoreCreateMutex();

  // init ble before rng
  BLEDevice::init(DEVICE_NAME);
  this->random_seed();

#ifdef HAL_M5STICK_C
  ENTER_CRITICAL;
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);

  // Set LED pins
  pinMode(10, OUTPUT);
  EXIT_CRITICAL;
#endif
}

void _TS_HAL::update()
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.update();
#endif
  EXIT_CRITICAL;
}



//
// Random
//

// seeds the randomizer based on hardware impl
// - ESP32 has a true rng if BT/Wifi is enabled, p-rng otherwise
void _TS_HAL::random_seed()
{
  ENTER_CRITICAL;
  randomSeed(esp_random());
  EXIT_CRITICAL;
}

// a random number between min and max-1 (long)
// - TODO: we theoretically can just use esp_random directly
uint32_t _TS_HAL::random_get(uint32_t min, uint32_t max)
{
  ENTER_CRITICAL;
  uint32_t val = random(min, max);
  EXIT_CRITICAL;
  return val;
}



//
// LCD
//

void _TS_HAL::lcd_brightness(uint8_t level)
{
  if (level > 100) level = 100;
  else if (level < 0) level = 0;

  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  // m5stickc valid levels are 7-15 for some reason
  // level / 12 -> (0-8), +7 -> 7-15
  M5.Axp.ScreenBreath(7 + (level / 12));
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_backlight(bool on)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Axp.SetLDO2(on);
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_sleep(bool enabled)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Axp.SetLDO2(!enabled);
  M5.Axp.SetLDO3(!enabled);
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_cursor(uint16_t x, uint16_t y)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  // x, y, font
  // We only use font 2 for now
  M5.Lcd.setCursor(x, y, 2);
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_printf(const char* t)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.printf(t);
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_printf(const char* t, int a, int b, int c)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.printf(t, a, b, c);
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_qrcode(const char *string, uint16_t x, uint16_t y, uint8_t width, uint8_t version)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.qrcode(string, x, y, width, version);
#endif
  EXIT_CRITICAL;
}

inline void _TS_HAL::lcd_qrcode(const String &string, uint16_t x, uint16_t y, uint8_t width, uint8_t version)
{
  lcd_qrcode(string.c_str(), x, y, width, version);
}

void _TS_HAL::lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, const uint16_t *data)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.drawBitmap(x0, y0, w, h, data);
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, const uint8_t *data)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.drawBitmap(x0, y0, w, h, data);
#endif
  EXIT_CRITICAL;
}

inline void _TS_HAL::lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, uint16_t *data)
{
  lcd_drawbitmap(x0, y0, w, h, (const uint16_t *)data);
}

inline void _TS_HAL::lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, uint8_t *data)
{
  lcd_drawbitmap(x0, y0, w, h, (const uint8_t *)data);
}

void _TS_HAL::lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, const uint16_t *data, uint16_t transparent)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.drawBitmap(x0, y0, w, h, data, transparent);
#endif
  EXIT_CRITICAL;
}



//
// RTC
//

void _TS_HAL::rtc_get(TS_DateTime &dt)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  RTC_TimeTypeDef RTC_TimeStruct;
  RTC_DateTypeDef RTC_DateStruct;
  M5.Rtc.GetTime(&RTC_TimeStruct);
  M5.Rtc.GetData(&RTC_DateStruct);
  dt.hour   = RTC_TimeStruct.Hours;
  dt.minute = RTC_TimeStruct.Minutes;
  dt.second = RTC_TimeStruct.Seconds;
  dt.day    = RTC_DateStruct.Date;
  dt.month  = RTC_DateStruct.Month;
  dt.year   = RTC_DateStruct.Year;
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::rtc_set(TS_DateTime &dt)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  RTC_TimeTypeDef TimeStruct;
  RTC_DateTypeDef DateStruct;
  TimeStruct.Hours   = dt.hour;
  TimeStruct.Minutes = dt.minute;
  TimeStruct.Seconds = dt.second;
  // DateStruct.WeekDay = 3; // TODO: do we even need this
  DateStruct.Month  = dt.month;
  DateStruct.Date   = dt.day;
  DateStruct.Year   = dt.year;
  M5.Rtc.SetTime(&TimeStruct);
  M5.Rtc.SetData(&DateStruct); // NOTE: apparently their lib has a typo
#endif
  EXIT_CRITICAL;
}



//
// Misc IO
//

void _TS_HAL::led_set(TS_Led led, bool enable)
{
#ifdef HAL_M5STICK_C
  switch (led) {
    case TS_Led::Red:
      digitalWrite(10, enable ? 0 : 1); // logic is reversed probably due to pulldown
      break;
  }
#endif
}

bool _TS_HAL::btn_a_get()
{
#ifdef HAL_M5STICK_C
  return M5.BtnA.read() == 1;
#endif
}


//
// BLE
//

void _TS_HAL::ble_init()
{
  if (!this->bleInitialized)
  {
    pBLEServer = BLEDevice::createServer();
    pBLEAdvertiser = BLEDevice::getAdvertising();

    this->pBLEScan = BLEDevice::getScan();  //create new scan
    this->pBLEScan->setActiveScan(true);    //active scan uses more power, but get results faster
    // pBLEScan->setInterval(100);    // in ms , left to defaults
    // pBLEScan->setWindow(99);

    this->bleInitialized = true;
  }
}

BLEScanResults _TS_HAL::ble_scan(uint8_t seconds)
{
  this->pBLEScan->clearResults();
  return this->pBLEScan->start(seconds, false);
}

BLEServer* _TS_HAL::ble_server_get()
{
  return this->pBLEServer;
}


//
// Power management
//

void _TS_HAL::sleep(TS_SleepMode sleepMode, uint32_t ms)
{
  switch (sleepMode)
  {
    case TS_SleepMode::Light:
#ifdef HAL_M5STICK_C
      M5.Axp.LightSleep(SLEEP_MSEC(ms));
#endif
      break;

    case TS_SleepMode::Deep:
      ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
      M5.Axp.DeepSleep(SLEEP_MSEC(ms));
      // Execution terminates here
#endif
      EXIT_CRITICAL;
      break;

    case TS_SleepMode::Task:
      vTaskDelay(ms / portTICK_PERIOD_MS);
      break;
      
    default:
      delay(ms);
  }
}

void _TS_HAL::power_off()
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Axp.PowerOff();
#endif
  // Execution terminates here
  EXIT_CRITICAL;
}

void _TS_HAL::reset()
{
  ENTER_CRITICAL;
  ESP.restart();
  // Execution terminates here
  EXIT_CRITICAL;
}

void _TS_HAL::power_set_mic(bool enabled)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  // GPIO0 low noise LDO
  M5.Axp.SetGPIO0(enabled);
#endif
  EXIT_CRITICAL;
}

//
// Common logging functions
//

void _TS_HAL::log_init()
{
#ifdef HAL_SERIAL_LOG
  Serial.begin(115200);
#endif
}



//
// Debug
//

// logs a failure message and reboots
void _TS_HAL::fail_reboot(const char *msg)
{
  log(msg);
  delay(3000);
  reset();
}
