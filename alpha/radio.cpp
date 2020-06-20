#include "radio.h"
#include "hal.h"

// WiFi functions, call wifi_connect() to connect to "Test" wifi, wifi_disconnect() to disconnect.
// Prints helpful debug to console upon call. 

#ifndef WIFI_SSID
#define WIFI_SSID "test"       // Enter your SSID here
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "password"    // Enter your WiFi password here
#endif
#define SSID_DISPLAY_COUNT 1
#define MAX_TRIES 10


_TS_RADIO TS_RADIO;

// Ctor
_TS_RADIO::_TS_RADIO() {
  this->wifiConnected = false;
}

void _TS_RADIO::begin()
{
  wifi_connect();
}

//
// Connect to WIFI
// Scans for SSIDs, if preset SSID is found, attempt to connect, 
// else prints the failed connection attempt. 
void _TS_RADIO::wifi_connect()
{
  uint8_t tries = 0;
  if (wifi_scan_networks())
  {
    while(!this->wifiConnected && tries < MAX_TRIES)
    {
      //Attempt to connect to 'WIFI_SSID', prints name of connected WIFI. 
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      // 2 second delay buffer to allow WiFi to connect 
      TS_HAL.sleep(TS_SleepMode::Task, 2000);
      if (WiFi.status() == WL_CONNECTED)
      {
        log_i("Successfully Connected to WIFI: %s", WiFi.SSID());
        this->wifiConnected = true;
        return;
      }

      log_i("Wifi connect tries: %d", tries);

      tries++;

      TS_HAL.sleep(TS_SleepMode::Task, 200);
    }

    log_i("Wifi connect MAX_TRIES reached");
  } 
  else 
  {
    log_i("Known SSID not detected");
  }
}

// disconnects from WIFI 
void _TS_RADIO::wifi_disconnect()
{
  WiFi.disconnect();
  TS_HAL.sleep(TS_SleepMode::Task, 1000);
  if (WiFi.status() != WL_CONNECTED) 
  {
    log_i("Not Connected to any WIFI");
    this->wifiConnected = false;
  }
  else
  {
    log_i("Unsuccessful Disconnection from WIFI");
  }
}

bool _TS_RADIO::wifi_is_connected()
{
  return this->wifiConnected;
}

// Returns true if network is available to connect
bool _TS_RADIO::wifi_scan_networks()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int SSID_COUNT = WiFi.scanNetworks();
  for(int i = 0; i < SSID_COUNT; i++)
  {
    if(WiFi.SSID(i) == WIFI_SSID)
    {
      return true;
    }
  }

  return false;
}
