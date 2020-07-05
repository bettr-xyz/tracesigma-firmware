#ifndef __TS_RADIO__
#define __TS_RADIO__

#define HAL_M5STICK_C
#define HAL_SERIAL_LOG

#define DEVICE_NAME "TraceStick V0.1"

class _TS_RADIO
{
  public:
    _TS_RADIO();

    void init();

    bool wifi_is_connected();
    void wifi_enable(bool);
    void wifi_update();

  private:
    void wifi_connect(char*, char*);
    void wifi_disconnect();
    bool wifi_scan_networks(char*);
    void download_temp_ids(char*);

    bool wifiEnabled;
    long wifiTimerStart;
};

extern _TS_RADIO TS_RADIO;

#endif
