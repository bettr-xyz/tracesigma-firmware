#ifndef __TS_RADIO__
#define __TS_RADIO__

#include <WiFi.h>

#define HAL_M5STICK_C
#define HAL_SERIAL_LOG

#define DEVICE_NAME "TraceStick V0.1"

class _TS_RADIO
{
  public:
    _TS_RADIO();

    void begin();

    void wifi_connect();
    void wifi_disconnect();

    bool wifi_is_connected();

    bool wifi_scan_networks();

  private:
    bool wifiConnected;
};

extern _TS_RADIO TS_RADIO;

#endif
