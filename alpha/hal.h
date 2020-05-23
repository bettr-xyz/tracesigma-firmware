//
// TraceStick: Hardware Abstraction Layer
// - All platform-specific function calls go here
// - Helps to port to different esp32-compatible platforms e.g. M5Stack Fire/Grey, or custom board
// - Expected to be ugly with precomp defs
// - This file is self-contained, lower level lib includes go into hal.cpp

#ifndef __TS_HAL__
#define __TS_HAL__

#include <stdint.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

//
// Platform-specific definitions
//
#define HAL_M5STICK_C
#define HAL_SERIAL_LOG

#define DEVICE_NAME "TraceStick V0.1"

struct TS_DateTime
{
  uint8_t hour, minute, second;
  uint8_t day, month;
  uint16_t year;
};

enum TS_SleepMode
{
  Default,  // most accurate, equivalent to delay
  Task,     // allow task scheduling, better for longer durations
  Light,    // less accurate, has wakeup delay
  Deep,     // suspends cpu, wake w/ reboot
};

enum TS_Led
{
  Red,
};

class _TS_HAL
{
  public:
    _TS_HAL();

    //
    // Platform-specific functionality wrappers
    //

    void begin();
    void update();



    //
    // Random
    // - depending on platform, implement seeding and random generator
    // - some platforms may have hardware rng

    // seeds the randomizer based on hardware impl
    void random_seed();

    // a random number between min and max-1 (long)
    uint32_t random_get(uint32_t, uint32_t);



    //
    // LCD
    //

    // sets brightness, 0-100
    void lcd_brightness(uint8_t);

    // turn on or off backlight
    void lcd_backlight(bool);

    // sleep/wake the lcd controls
    void lcd_sleep(bool);

    // sets the cahracter cursor to a pixel position
    void lcd_cursor(uint16_t, uint16_t);

    // TODO: for some reason templates aren't working with .ino
    void lcd_printf(const char*);
    void lcd_printf(const char*, int);
    void lcd_printf(const char*, int, int, int);

    // proxy display calls
    void lcd_qrcode(const char *string, uint16_t x = 5, uint16_t y = 5, uint8_t width = 70, uint8_t version = 7);
    inline void lcd_qrcode(const String &string, uint16_t x = 5, uint16_t y = 5, uint8_t width = 70, uint8_t version = 7);
    void lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, const uint16_t *data);
    void lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, const uint8_t *data);
    inline void lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, uint16_t *data);
    inline void lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, uint8_t *data);
    void lcd_drawbitmap(int16_t x0, int16_t y0, int16_t w, int16_t h, const uint16_t *data, uint16_t transparent);



    //
    // RTC
    //
    void rtc_get(TS_DateTime &);
    void rtc_set(TS_DateTime &);



    //
    // Misc IO
    //
    void led_set(TS_Led, bool);
    bool btn_a_get();
    bool btn_b_get();


    //
    // BLE
    //
    void ble_init();
    BLEServer* ble_server_get();
    BLEScanResults ble_scan(uint8_t seconds);

    
    //
    // Power management
    //
    void sleep(TS_SleepMode, uint32_t);
    void power_off();
    void reset();
    void power_set_mic(bool);

    
    //
    // Common logging functions
    //
    void log_init();

    // Logs a line, similar to println
    template<typename T> inline void log(T val)
    {
    #ifdef HAL_SERIAL_LOG
      Serial.println(val);
    #endif
    };

    // Logs without newline
    template<typename T> inline void logcat(T val)
    {
    #ifdef HAL_SERIAL_LOG
      Serial.write(val);
    #endif
    };



    //
    // Debug
    //

    // logs a failure message and reboots
    void fail_reboot(const char*);

  private:
    bool            bleInitialized;
    BLEScan*        pBLEScan;
    BLEServer*      pBLEServer;
    BLEAdvertising* pBLEAdvertiser;
};

extern _TS_HAL TS_HAL;

#endif
