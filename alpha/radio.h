//
// TraceStick: Radio file
// - All platform-specific function calls go here

#ifndef __TS_RADIO__
#define __TS_RADIO__

//
// Platform-specific definitions
//
#define HAL_M5STICK_C
#define HAL_SERIAL_LOG

#define DEVICE_NAME "TraceStick V0.1"


class _TS_RADIO
{
  public:
    _TS_RADIO();

    //
    // Platform-specific functionality wrappers
    //

    void begin();

  //
  // WIFI 
  //
  void wifi_connect();

  private:
    bool            wifiInitialized;
};

extern _TS_RADIO TS_RADIO;

#endif
