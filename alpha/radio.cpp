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

#define WIFI_RETRY_INTERVAL 30000

_TS_RADIO TS_RADIO;

// Ctor
_TS_RADIO::_TS_RADIO() {
  this->wifiEnabled = false;
  this->wifiTimerStart = -(WIFI_RETRY_INTERVAL);
}

//
// Connect to WIFI
// Scans for SSIDs, if preset SSID is found, attempt to connect, 
// else prints the failed connection attempt. 
void _TS_RADIO::wifi_connect()
{
  if (wifi_scan_networks())
  {
    //Attempt to connect to 'WIFI_SSID', prints name of connected WIFI. 
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    log_i("Connecting to WIFI: %s", WiFi.SSID());
  } 
  else 
  {
    log_i("Known SSID not detected");
  }
}

void _TS_RADIO::wifi_disconnect()
{
  WiFi.disconnect();
}

bool _TS_RADIO::wifi_is_connected()
{  
  return WiFi.status() == WL_CONNECTED;
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
      log_i("Known networks are available");
      return true;
    }
  }

  log_i("No known SSID found");

  return false;
}

void _TS_RADIO::wifi_enable(bool enable)
{
  this->wifiEnabled = enable;
}

void _TS_RADIO::wifi_update()
{
  if (this->wifiEnabled)
  {
    if (!wifi_is_connected())
    {
      if (millis() - this->wifiTimerStart > WIFI_RETRY_INTERVAL)
      {
        log_i("Connecting to WiFi.");
        TS_RADIO.wifi_connect();
        this->wifiTimerStart = millis();
      }
      else
      {
        log_i("Waiting for WiFi interval");
      }
    }
  }
  else
  {
    if (wifi_is_connected())
    {
      log_i("Disconnecting WiFi.");
      TS_RADIO.wifi_disconnect();
    }
  }
  
}