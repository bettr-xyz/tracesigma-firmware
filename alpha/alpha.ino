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

  xTaskCreate(
    UITask, /* Task function. */
    "UI", /* name of task. */
    1000, /* Stack size of task */
    NULL, /* parameter of the task */
    3, /* priority of the task */
    NULL); /* Task handle to keep track of created task */

  xTaskCreate(
    traceTask, /* Task function. */
    "Trace", /* name of task. */
    10000, /* Stack size of task */
    NULL, /* parameter of the task */
    2, /* priority of the task */
    NULL); /* Task handle to keep track of created task */
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

}

void UITask (void* parameter)
{
  /* loop forever */
  for(;;)
  {
    
  }
  /* delete a task when finish,
  this will never happen because this is an infinite loop */
  vTaskDelete(NULL);
}

void traceTask(void* parameter)
{
  uint16_t connectedCount;
  uint16_t sleepDuration;
  for(;;)
  {
    // blink once a second
    TS_HAL.setLed(TS_Led::Red, true);
    delay(1000);
    TS_HAL.setLed(TS_Led::Red, false);
    delay(1000);
    
    // don't turn off radio if we have connected clients
    connectedCount = OT_ProtocolV2.get_connected_count();
    sleepDuration = TS_HAL.random_get(1000, 3000);
  
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
  /* delete a task when finish,
  this will never happen because this is an infinite loop */
  vTaskDelete(NULL);
}
