#include "cleanbox.h"
#include "hal.h"
#include "opentracev2.h"

// Notes:
// - look at mods/boards.diff.txt -- set CPU to 80mhz instead of 240mhz
//

bool powerSaveTest = false;

void setup() {
  TS_HAL.begin();
  TS_HAL.ble_init();
  
  // Reduce screen brightness to minimum visibility to reduce power consumption
  TS_HAL.lcd_brightness(12);

  TS_HAL.lcd_cursor(40, 0);
  TS_HAL.lcd_printf("ALPHA TEST");

  if(powerSaveTest)
  {
    TS_HAL.lcd_backlight(false);
    TS_HAL.lcd_sleep(true);
  }

  OT_ProtocolV2.begin();
  
}

int skips = 0;

void loop() {
  TS_HAL.update();

  if(!powerSaveTest)
  {
    TS_DateTime datetime;
    TS_HAL.rtc_get(datetime);
    
    TS_HAL.lcd_cursor(0, 15);
    // TODO: does not work with F()
    TS_HAL.lcd_printf("Date: %04d-%02d-%02d\n",     datetime.year, datetime.month, datetime.day);
    TS_HAL.lcd_printf("Time: %02d : %02d : %02d\n", datetime.hour, datetime.minute, datetime.second);
  }

  // blink once a second
  TS_HAL.setLed(TS_Led::Red, true);
  TS_HAL.sleep(TS_SleepMode::Default, 1);
  TS_HAL.setLed(TS_Led::Red, false);

  // don't turn off radio if we have connected clients
  uint16_t connectedCount = OT_ProtocolV2.get_connected_count();
  uint16_t sleepDuration = TS_HAL.random_get(1000, 3000);

  Serial.print(F("Devices connected: "));
  Serial.println(connectedCount);
  if(connectedCount > 0) {
    TS_HAL.sleep(TS_SleepMode::Task, sleepDuration);
  } else {
    TS_HAL.sleep(TS_SleepMode::Light, sleepDuration);
  }

  if(skips >= 5) { // vary the interval between scans here
    skips = 0;
    
    // spend up to 1s scanning, lowest acceptable rssi: -95
    OT_ProtocolV2.scan_and_connect(1, -95);
  }
  else
  {
    ++skips;
  }

  // enable advertising
  OT_ProtocolV2.advertising_start();
  // just advertise for 1s
  TS_HAL.sleep(TS_SleepMode::Task, 1000);
  // disable advertising, get back to sleep
  OT_ProtocolV2.advertising_stop();

  // Give some time for comms after broadcasts
  // TODO: by right should wait T time after last uncompleted handshake before going back to sleep
  TS_HAL.sleep(TS_SleepMode::Task, 100);

  // TODO: call OT update_characteristic_cache at least once every 15 mins
}















