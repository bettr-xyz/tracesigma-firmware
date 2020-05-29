#include "radio.h"
#include "esp_pm.h"

// Lower level library include decisions go here

#ifdef HAL_M5STICK_C
#include <M5StickC.h>
#include "AXP192.h"

#include <WiFi.h>
#define WIFI_SSID "Test"       // Enter your SSID here
#define WIFI_PASS "password"    // Enter your WiFi password here
#define SSID_DISPLAY_COUNT 3
#elif HAL_M5STACK
#include <M5Stack.h>
#endif

#define ENTER_CRITICAL  xSemaphoreTake(halMutex, portMAX_DELAY)
#define EXIT_CRITICAL   xSemaphoreGive(halMutex)
static SemaphoreHandle_t halMutex;

// Your one and only
_TS_RADIO TS_RADIO;
_TS_RADIO::_TS_RADIO() {}

void _TS_RADIO::begin()
{
  this->wifiInitialized = false;
}

//
// WIFI setup
// 
void _TS_RADIO::wifi_connect()
{
  if (!this->wifiInitialized)
  {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    int SSID_COUNT = 0;
    SSID_COUNT = WiFi.scanNetworks();
    if (SSID_COUNT > 3)
    {
    // limit maximum number of SSID networks displayed.
      SSID_COUNT = SSID_DISPLAY_COUNT;
    }
    for (int i = 0; i < SSID_COUNT; ++i)
    {
      //prints top 3 nearest WIFI SSIDS
      //log(WiFi.SSID(i));
      Serial.println(WiFi.SSID(i));
      //sleep(TS_SleepMode2::Task2, 100);
    }
  
    if (SSID_COUNT == 0)
    {
      //log("No WIFI Networks Found \n");
    }
    else
    {
      //Connect to, prints name of connected WIFI. 
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      //sleep(TS_SleepMode2::Task2, 100);
      delay(2000);
      if (WiFi.status() != WL_CONNECTED) 
      {

       // log("Not Connected");
      }
      else if (WiFi.status() == WL_CONNECTED)
       // log("Connected");
        //log(WiFi.SSID());
        Serial.println(WiFi.SSID());
        this->wifiInitialized = true;
    }
  }
}
