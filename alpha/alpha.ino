#include "cleanbox.h" // This has to be first
#include "tests.h"    // This has to be second

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


void setup() {
  TS_HAL.begin();

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
  log_i("Setup completed free heap: %d", ESP.getFreeHeap());

#ifdef TESTDRIVER

  TS_StorageTests.run_all_repeatedly();

#endif
}

int skips = 0;

void loop() {
  TS_HAL.update();
  TS_POWER.update();

  // Wifi 
  TS_RADIO.wifi_enable(TS_POWER.get_state() == TS_PowerState::HIGH_POWER);
  TS_RADIO.wifi_update();

  if (TS_HAL.ble_is_init())
  {
    // blink once a second
    TS_HAL.led_set(TS_Led::Red, true);
    TS_HAL.sleep(TS_SleepMode::Default, 1);
    TS_HAL.led_set(TS_Led::Red, false);

    OT_ProtocolV2.update();
    
  }

  // Give some time for comms after broadcasts
  // TODO: by right should wait T time after last uncompleted handshake before going back to sleep
  TS_HAL.sleep(TS_SleepMode::Task, 100);

  // TODO: call OT update_characteristic_cache at least once every 15 mins
}
