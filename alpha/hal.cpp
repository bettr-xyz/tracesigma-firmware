#include "hal.h"
#include "esp_pm.h"

// Lower level library include decisions go here

#ifdef HAL_M5STICK_C

#include <M5StickC.h>
#include "AXP192.h"
#include "driver/uart.h"

#define BUTTONP 35
#define BUTTONA 37
#define BUTTONB 39

#elif HAL_M5STACK

// #include <M5Stack.h>

#endif

#define CONFIG_ESP_CONSOLE_UART_NUM 0

#define ENTER_CRITICAL  xSemaphoreTake(halMutex, portMAX_DELAY)
#define EXIT_CRITICAL   xSemaphoreGive(halMutex)
static SemaphoreHandle_t halMutex;

// Persistent memory
// - Retained across reboots
// - Lost during powerdown
RTC_NOINIT_ATTR _TS_PersistMem TS_PersistMem;

// Your one and only
_TS_HAL TS_HAL;
_TS_HAL::_TS_HAL() {}

void _TS_HAL::begin()
{
  persistmem_init();
  this->uart_init();
  this->bleInitialized = false;
  halMutex = xSemaphoreCreateMutex();

  // init ble before rng
  this->ble_init();
  this->random_seed();

#ifdef HAL_M5STICK_C
  ENTER_CRITICAL;
  
  // don't enable serial by default
  M5.begin(true, true, false);

  //
  // Configure power options
  //
  M5.Axp.SetChargeCurrent(CURRENT_100MA);  // Default is 100mA
  M5.Axp.SetChargeVoltage(VOLTAGE_4150MV); // Default is 4200mV
  M5.Axp.SetAdcRate(ADC_RATE_025HZ);       // Default sample rate is 200Hz
  M5.Axp.SetVOff(VOLTAGE_OFF_3200MV);      // Default is 3000mV

  // Default screen options
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);

  // Set LED pins
  pinMode(10, OUTPUT);

  // init buttons
  btn_init();
  
  EXIT_CRITICAL;
#endif

  // disable power to microphone
  power_set_mic(false);
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
  // m5stickc valid levels are 7-12 for some reason
  // level / 20 -> (0-5), +7 -> 7-12
  M5.Axp.ScreenBreath(7 + (level / 20));
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

void _TS_HAL::lcd_printf(const char* t, const char* a)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.printf(t, a);
#endif
  EXIT_CRITICAL;
}

void _TS_HAL::lcd_printf(const char* t, int a)
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.printf(t, a);
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

void _TS_HAL::lcd_clear()
{
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  M5.Lcd.fillScreen(BLACK);
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

TS_ButtonState btn_handle_power()
{
  TS_ButtonState state = TS_ButtonState::Short;

  #ifdef HAL_M5STICK_C
  ENTER_CRITICAL;
  uint8_t readBtn = M5.Axp.GetBtnPress();
  EXIT_CRITICAL;
  switch (readBtn)
  {
    case 0x00:
      state = TS_ButtonState::NotPressed;
      break;
    case 0x01:
      state = TS_ButtonState::Long;
      break;
    case 0x02:
      state = TS_ButtonState::Short;
      break;
  }
  #endif

  return state;
}

void _TS_HAL::btn_init()
{
  this->buttonA = new TS_IOButton(BUTTONA, nullptr);
  this->buttonB = new TS_IOButton(BUTTONB, nullptr);
  this->buttonP = new TS_IOButton(BUTTONP, btn_handle_power); 

  #ifdef HAL_M5STICK_C
  // To read interrupts from AXP192
  M5.MPU6886.setIntActiveLow();
  M5.Axp.ClearIRQ();
  #endif
}

TS_ButtonState _TS_HAL::btn_a_get()
{
  return this->buttonA->get_state();
}

TS_ButtonState _TS_HAL::btn_b_get()
{
  return this->buttonB->get_state();
}

TS_ButtonState _TS_HAL::btn_power_get()
{
  return this->buttonP->get_state();
}

void _TS_HAL::uart_init()
{
  /* Configure UART. Note that REF_TICK is used so that the baud rate remains
   * correct while APB frequency is changing in light sleep mode.
   */
  uart_config_t uart_config;
  uart_config.baud_rate     = 115200;
  uart_config.data_bits     = UART_DATA_8_BITS;
  uart_config.parity        = UART_PARITY_DISABLE;
  uart_config.stop_bits     = UART_STOP_BITS_1;
  uart_config.use_ref_tick  = true;
  uart_config.flow_ctrl     = UART_HW_FLOWCTRL_DISABLE;
  uart_config.rx_flow_ctrl_thresh = 0;
  
  ESP_ERROR_CHECK( uart_param_config((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

  /* Install UART driver for interrupt-driven reads and writes */
  ESP_ERROR_CHECK( uart_driver_install((uart_port_t)CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
}



//
// BLE
//

void _TS_HAL::ble_init()
{
  if (!this->bleInitialized)
  {
    BLEDevice::init(DEVICE_NAME);
  
    pBLEServer = BLEDevice::createServer();
    pBLEAdvertiser = BLEDevice::getAdvertising();

    this->pBLEScan = BLEDevice::getScan();  //create new scan
    this->pBLEScan->setActiveScan(true);    //active scan uses more power, but get results faster
    // pBLEScan->setInterval(100);    // in ms , left to defaults
    // pBLEScan->setWindow(99);

    this->bleInitialized = true;
  }
}

void _TS_HAL::ble_deinit()
{
  if (this->bleInitialized)
  {
    BLEDevice::deinit();
    this->bleInitialized = false;
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

bool _TS_HAL::ble_is_init()
{
  return this->bleInitialized;
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
  TS_PersistMem.gracefulShutdown = true;

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

uint8_t _TS_HAL::power_get_batt_level()
{
  long level;
#ifdef HAL_M5STICK_C
  ENTER_CRITICAL;
  level = M5.Axp.GetVbatData();
  EXIT_CRITICAL;
  level = map(level * 1.1, 3100, 4000, 0, 100);
#endif
  return constrain(level, 0, 100);
}

bool _TS_HAL::power_is_charging()
{
  uint8_t is_charging;
  ENTER_CRITICAL;
#ifdef HAL_M5STICK_C
  is_charging = M5.Axp.GetBatteryChargingStatus() & (1 << 6);
#endif
  EXIT_CRITICAL;
  return is_charging;
}



//
// Persistmem
//

void _TS_HAL::persistmem_init()
{
  if(TS_PersistMem.validStart != TS_PERSISTMEM_VALID || TS_PersistMem.validEnd != TS_PERSISTMEM_VALID)
  {
    // clear all counters
    memset(&TS_PersistMem, 0, sizeof(TS_PersistMem));
    TS_PersistMem.validStart = TS_PERSISTMEM_VALID;
    TS_PersistMem.validEnd = TS_PERSISTMEM_VALID;
  }
  else if(TS_PersistMem.gracefulShutdown == false)
  {
    ++TS_PersistMem.crashCount;
  }

  // reset this after every reboot
  TS_PersistMem.gracefulShutdown = false;
}


//
// Debug
//

// logs a failure message and reboots
void _TS_HAL::fail_reboot(const char *msg)
{
  log_e("%s", msg);
  delay(3000);
  reset();
}
