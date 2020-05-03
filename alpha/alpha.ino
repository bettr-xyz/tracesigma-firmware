#include "hal.h"
#include "opentracev2.h"

// Notes:
// - look at mods/boards.diff.txt -- set CPU to 80mhz instead of 240mhz
//

bool powerSaveTest = false;
bool bleEnabled = true;

void setup() {
  TS_HAL.begin();

  // Reduce screen brightness to minimum visibility to reduce power consumption
  TS_HAL.lcd_brightness(12);

  TS_HAL.lcd_cursor(40, 0);
  TS_HAL.lcd_printf("RTC TEST");

  if(powerSaveTest)
  {
    TS_HAL.lcd_backlight(false);
    TS_HAL.lcd_sleep(true);
  }

  if(bleEnabled)
  {
    TS_HAL.ble_init();
  }
}

void loop() {
  TS_HAL.update();

  if(!powerSaveTest)
  {
    TS_DateTime datetime;
    TS_HAL.rtc_get(datetime);
    
    TS_HAL.lcd_cursor(0, 15);
    TS_HAL.lcd_printf("Date: %04d-%02d-%02d\n",     datetime.year, datetime.month, datetime.day);
    TS_HAL.lcd_printf("Time: %02d : %02d : %02d\n", datetime.hour, datetime.minute, datetime.second);
  }

  // blink once a second
  TS_HAL.setLed(TS_Led::Red, true);
  TS_HAL.sleep(TS_SleepMode::Light, 1);
  TS_HAL.setLed(TS_Led::Red, false);
  
  TS_HAL.sleep(TS_SleepMode::Light, 999);

  if(bleEnabled)
  {
    BLEScanResults results = TS_HAL.ble_scan(1);
    uint16_t deviceCount = results.getCount();
    Serial.print("Devices found: ");
    Serial.println(deviceCount);
    for (uint32_t i = 0; i < deviceCount; i++)
    {
      BLEAdvertisedDevice device = results.getDevice(i);
      Serial.println(device.toString().c_str());
    }
  }
}
