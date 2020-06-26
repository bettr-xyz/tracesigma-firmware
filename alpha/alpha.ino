#include "cleanbox.h"
#include "hal.h"
#include "radio.h"
#include "ui.h"
#include "power.h"
#include "opentracev2.h"
#include "serial_cmd.h"
#include "storage.h"

// Notes:
// - look at mods/boards.diff.txt -- set CPU to 80mhz instead of 240mhz
//

bool powerSaveTest = false;

void setup() {
  TS_HAL.begin();
  TS_HAL.ble_init();

  TS_Storage.begin();
  log_w("Storage free: %d, %d%", TS_Storage.freespace_get(), TS_Storage.freespace_get_pct());

  OT_ProtocolV2.begin();

  TS_POWER.init();
  
  // This starts a new task
  TS_UI.begin();

  // Start serial command console
  TS_SerialCmd.init();
  TS_SerialCmd.begin();

  log_w("Crash count: %d", TS_PersistMem.crashCount);
}

int skips = 0;

void loop() {
  TS_HAL.update();
  TS_POWER.update();

  // blink once a second
  TS_HAL.led_set(TS_Led::Red, true);
  TS_HAL.sleep(TS_SleepMode::Default, 1);
  TS_HAL.led_set(TS_Led::Red, false);

  // Wifi 
  TS_RADIO.wifi_enable(TS_POWER.get_state() == TS_PowerState::HIGH_POWER);
  TS_RADIO.wifi_update();


  
  // don't turn off radio if we have connected clients
  uint16_t connectedCount = OT_ProtocolV2.get_connected_count();
  uint16_t sleepDuration = TS_HAL.random_get(1000, 3000);

  log_i("Devices connected: %d", connectedCount);
  
  if(connectedCount > 0) {
    TS_HAL.sleep(TS_SleepMode::Task, sleepDuration);
  } else {
    // TODO: figure out how to sleep deeper
    // TS_HAL.sleep(TS_SleepMode::Light, sleepDuration);
    TS_HAL.sleep(TS_SleepMode::Task, sleepDuration);
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
