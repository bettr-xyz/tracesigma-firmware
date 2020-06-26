#include "radio.h"
#include "hal.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// WiFi functions, call wifi_connect() to connect to "Test" wifi, wifi_disconnect() to disconnect.
// Prints helpful debug to console upon call. 

#define SSID_DISPLAY_COUNT 1

#define WIFI_RETRY_INTERVAL 30000

#define TEMPID_BATCH_SIZE 100
#define HOST "asia-east2-bettr-trace-1.cloudfunctions.net"
#define URI_GETTEMPIDS "/getTempIDs"
#define HTTPS_PORT 443
#define ROOT_CA \
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
    "-----END CERTIFICATE-----\n"

_TS_RADIO TS_RADIO;

// Ctor
_TS_RADIO::_TS_RADIO() {
  this->wifiEnabled = false;
  this->wifiTimerStart = -(WIFI_RETRY_INTERVAL);

  strcpy(this->ssid, "");
  strcpy(this->password, "");
}

//
// Connect to WIFI
// Scans for SSIDs, if preset SSID is found, attempt to connect, 
// else prints the failed connection attempt. 
void _TS_RADIO::wifi_connect()
{
  log_i("ESP FREE HEAP: %d", ESP.getFreeHeap());
  if (wifi_scan_networks())
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
    } else {
      log_i("Wifi is connected, downloading ids");
      download_temp_ids();
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

void _TS_RADIO::wifi_config(char* ssid, char* password)
{
  strcpy(this->ssid, ssid);
  strcpy(this->password, password);
}

void _TS_RADIO::download_temp_ids()
{
  this->client.setCACert(ROOT_CA);
  log_i("Connecting to %s", HOST);
  if (!this->client.connect(HOST, HTTPS_PORT)) 
  { 
    log_e("Connection to %s failed", HOST);
    return;
  } 

  const char* body = "{\"data\":{\"uid\":\"12345678901234567890123456789012\"}}";

  String postRequest = 
    String("POST ") + URI_GETTEMPIDS + " HTTP/1.1\r\n" + 
    "Host: " + HOST + "\r\n" + 
    "Content-Type: application/json\r\n" + 
    "Content-Length: " + strlen(body) + "\r\n" + 
    "\r\n" + body;

  this->client.print(postRequest);

  if (this->client.println() == 0) {
    log_e("Failed to send request");
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  this->client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    log_e("Unexpected response %s", status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!this->client.find(endOfHeaders)) {
    log_e("Invalid response");
    return;
  }
  
  const size_t capacity = JSON_ARRAY_SIZE(TEMPID_BATCH_SIZE) + JSON_OBJECT_SIZE(1) + (TEMPID_BATCH_SIZE + 1)*JSON_OBJECT_SIZE(3) + 12610; // 17806

  DynamicJsonDocument doc(capacity);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, this->client);
  if (error) {
    log_e("deserializeJson() failed: %s", error.c_str());
    return;
  }
 
  JsonObject result = doc["result"];
  const char* result_status = result["status"]; // "SUCCESS"

  if (strcmp(result_status, "SUCCESS") != 0)
  {
    log_e("Error in JSON result status");
  }
  else
  {
    log_i("***Request status: OK***");

    log_i("======RESPONSE BODY======");
    
    JsonArray result_tempIDs = result["tempIDs"];
    JsonObject result_tempIDs_0 = result_tempIDs[0];
    const char* result_tempIDs_0_tempID = result_tempIDs_0["tempID"]; // "sea08KGxj267W31kxV4K2y9WiUsCN000G8rlLHHi+0Ha0l4QvOMIuS5ef5cYPg+Inr+7aDYISbbAcu+vJw=="
    long result_tempIDs_0_startTime = result_tempIDs_0["startTime"]; // 1593146131
    long result_tempIDs_0_expiryTime = result_tempIDs_0["expiryTime"]; // 1593147031
    
    log_i("tempID_0 id: %s", result_tempIDs_0_tempID);
    log_i("tempID_0 start: %ld", result_tempIDs_0_startTime);
    log_i("tempID_0 expiry: %ld", result_tempIDs_0_expiryTime);
  }

  // Disconnect
  client.stop();
}