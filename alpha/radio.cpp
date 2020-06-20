#include "radio.h"

// WiFi functions, call wifi_connect() to connect to "Test" wifi, wifi_disconnect() to disconnect.
// Prints helpful debug to console upon call. 

#define WIFI_SSID "Test"       // Enter your SSID here
#define WIFI_PASS "password"    // Enter your WiFi password here
#define SSID_DISPLAY_COUNT 1


_TS_RADIO TS_RADIO;

// Ctor
_TS_RADIO::_TS_RADIO() {
	this->wifiInitialized = false;
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
  if (!this->wifiInitialized)
  {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int SSID_COUNT = 0;
    bool SSID_FOUND = false;
    SSID_COUNT = WiFi.scanNetworks();
    for(int i = 0; i<SSID_COUNT; i++)
    {
      if(WiFi.SSID(i) == "Test")
      {
        SSID_FOUND = true; 
        log_i("SSID Test is found");
      }
    }
    if (SSID_FOUND)
    {
      //Attempt to connect to 'WIFI_SSID', prints name of connected WIFI. 
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      //sleep(TS_SleepMode2::Task2, 100);
      // 2 second delay buffer to allow WiFi to connect 
      delay(2000);
      if (WiFi.status() != WL_CONNECTED) 
      {
        log_i("Failed to Connect to WIFI");
      }
      else if (WiFi.status() == WL_CONNECTED)
      {
        log_i("Successfully Connected to WIFI");
        log_i("%s", WiFi.SSID());
        this->wifiInitialized = true;
      }
    }
    else if (!SSID_FOUND)
    {
      log_i("No WIFI Networks Available");
    }
  }
}

// disconnects from WIFI 
void _TS_RADIO::wifi_disconnect()
{
  WiFi.disconnect();
  delay(1000);
  if (WiFi.status() != WL_CONNECTED) 
    {
      log_i("Not Connected to any WIFI");
      this->wifiInitialized = false;
    }
  else
    {
      log_i("Unsuccessful Disconnection from WIFI");
    }
}
