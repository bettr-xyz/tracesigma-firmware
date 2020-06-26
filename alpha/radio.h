#ifndef __TS_RADIO__
#define __TS_RADIO__

#include <WiFiClientSecure.h>

#define HAL_M5STICK_C
#define HAL_SERIAL_LOG

#define DEVICE_NAME "TraceStick V0.1"

class _TS_RADIO
{
  public:
    _TS_RADIO();

    void begin();

    bool wifi_is_connected();
    void wifi_enable(bool);
    void wifi_update();
    void wifi_config(char*, char*);

  private:
    void wifi_connect();
    void wifi_disconnect();
    bool wifi_scan_networks();
    void download_temp_ids();

    bool wifiEnabled;
    long wifiTimerStart;

    char ssid[32];
    char password[32];

    WiFiClientSecure client;
};

extern _TS_RADIO TS_RADIO;

#endif
