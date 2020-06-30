#include "radio.h"
#include "hal.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "opentracev2.h"
#include "storage.h"

// WiFi functions, call wifi_connect() to connect to "Test" wifi, wifi_disconnect() to disconnect.
// Prints helpful debug to console upon call. 

#define SSID_DISPLAY_COUNT 1

#define WIFI_RETRY_INTERVAL 5000

#define TEMPID_BATCH_SIZE 100
#define HTTPS_PORT 443
const PROGMEM char* host = "asia-east2-bettr-trace-1.cloudfunctions.net";
const PROGMEM char* uri_getTempIDs = "/getTempIDs";
const PROGMEM char* endOfHeaders = "\r\n\r\n";
const PROGMEM char* root_ca = \
    "-----BEGIN CERTIFICATE-----\n" \
    "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n" \
    "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n" \
    "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n" \
    "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n" \
    "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n" \
    "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n" \
    "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n" \
    "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n" \
    "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n" \
    "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n" \
    "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n" \
    "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n" \
    "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n" \
    "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n" \
    "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n" \
    "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n" \
    "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n" \
    "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n" \
    "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n" \
    "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n" \
    "-----END CERTIFICATE-----\n";

_TS_RADIO TS_RADIO;

// Ctor
_TS_RADIO::_TS_RADIO() {
  this->wifiEnabled = false;
  this->wifiTimerStart = -(WIFI_RETRY_INTERVAL);

  strcpy(this->ssid, "");
  strcpy(this->password, "");
  strcpy(this->uid, "");
}

void _TS_RADIO::init() {
  TS_Settings* settings = TS_Storage.settings_get();
  this->config(settings->wifiSsid, settings->wifiPass, settings->userId);
}

//
// Connect to WIFI
// Scans for SSIDs, if preset SSID is found, attempt to connect, 
// else prints the failed connection attempt. 
void _TS_RADIO::wifi_connect()
{
  if (this->wifi_scan_networks())
  {
    //Attempt to connect to 'WIFI_SSID', prints name of connected WIFI. 
    WiFi.begin(this->ssid, this->password);
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
    if(strcmp(WiFi.SSID(i).c_str(), this->ssid) == 0)
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
  log_d("ESP FREE HEAP: %d", ESP.getFreeHeap());
    
  if (this->wifiEnabled)
  {
    if (!this->wifi_is_connected())
    {
      if (millis() - this->wifiTimerStart > WIFI_RETRY_INTERVAL)
      {
        log_i("Connecting to WiFi.");
        this->wifi_connect();
        this->wifiTimerStart = millis();
      }
      else
      {
        log_i("Waiting for WiFi interval");
      }
    } else {
      log_i("Wifi is connected, deinitializing BLE for RAM");
      log_d("Before ble deinit: %d", ESP.getFreeHeap());
      TS_HAL.ble_deinit();
      log_d("After ble deinit: %d", ESP.getFreeHeap());
      log_i("Downloading tempIds from server");
      this->download_temp_ids();
    }
  }
  else
  {
    if (wifi_is_connected())
    {
      log_i("Disconnecting WiFi.");
      this->wifi_disconnect();
      log_i("Initializing BLE.");
      TS_HAL.ble_init();
    }
  }
  
}

void _TS_RADIO::config(char* ssid, char* password, char* uid)
{
  strcpy(this->ssid, ssid);
  strcpy(this->password, password);
  strcpy(this->uid, uid);
}

void _TS_RADIO::download_temp_ids()
{
  log_d("BEGIN LEAKY: %d", ESP.getFreeHeap());
  WiFiClientSecure *client = new WiFiClientSecure;
  if (client)
  {
    client->setCACert(root_ca);
    
    log_d("Connecting to %s", host);
    if (!client->connect(host, HTTPS_PORT)) 
    { 
      log_e("Connection to %s failed", host);
      return;
    }

    char body[52];
    sprintf(body, "{\"data\":{\"uid\":\"%s\"}}", this->uid);

    String postRequest = 
      String("POST ") + uri_getTempIDs + " HTTP/1.1\r\n" + 
      "Host: " + host + "\r\n" + 
      "Content-Type: application/json\r\n" + 
      "Content-Length: " + strlen(body) + "\r\n" + 
      "\r\n" + body;

    client->print(postRequest);

    if (client->println() == 0) {
      log_e("Failed to send request");
      client->stop();
      return;
    }

    // Check HTTP status
    char status[32] = {0};
    client->readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      log_e("Unexpected response %s", status);
      client->stop();
      return;
    }

    // Skip HTTP headers
    if (!client->find(endOfHeaders)) {
      log_e("Invalid response");
      client->stop();
      return;
    }
    
    const size_t capacity = JSON_ARRAY_SIZE(TEMPID_BATCH_SIZE) + JSON_OBJECT_SIZE(1) + (TEMPID_BATCH_SIZE + 1)*JSON_OBJECT_SIZE(3) + 12610;

    DynamicJsonDocument doc(capacity);

    // Parse JSON object
    DeserializationError error = deserializeJson(doc, *client);
    if (error) 
    {
      log_e("deserializeJson() failed: %s", error.c_str());
      client->stop();
      return;
    }
  
    JsonObject result = doc["result"];
    const char* result_status = result["status"];

    if (strcmp(result_status, "SUCCESS") != 0)
    {
      log_e("Error in JSON result status");
    }
    else
    {
      log_d("***Request status: OK***");
      
      JsonArray result_tempIDs = result["tempIDs"];

      OT_TempID tempIds[OT_TEMPID_MAX];

      JsonObject result_tempIDs_0 = result_tempIDs[0];
      const char* result_tempIDs_0_tempID = result_tempIDs_0["tempID"];
      long result_tempIDs_0_startTime = result_tempIDs_0["startTime"]; 
      long result_tempIDs_0_expiryTime = result_tempIDs_0["expiryTime"];
    
      log_d("tempID_0 id: %s", result_tempIDs_0_tempID);
      log_d("tempID_0 start: %ld", result_tempIDs_0_startTime);
      log_d("tempID_0 expiry: %ld", result_tempIDs_0_expiryTime);

      log_d("Saving TempIDs to storage");
      if(TS_Storage.file_ids_writeall(OT_TEMPID_MAX, tempIds) != OT_TEMPID_MAX)
      {
        log_e("Error saving TempIDs");
      }
    }

    // Disconnect
    client->stop();
    delete client;
  }
  log_d("END LEAKY: %d", ESP.getFreeHeap());
}