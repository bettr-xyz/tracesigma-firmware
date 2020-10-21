#include "cleanbox.h"
#include "hal.h"
#include "storage.h"
#include "opentracev2.h"
#include "power.h"

// For Base64 encode
extern "C"
{
  #include "crypto/base64.h"
}

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <ArduinoJson.h>

//
// Defines
//

#define DEBUG_TIMINGS

#ifdef DEBUG_TIMINGS
  #define SCAN_INTERVAL_SECS 10
#else
  #define SCAN_INTERVAL_SECS 57
#endif

#define SLEEP_MIN 1900
#define SLEEP_MAX 2300
#define ADVERTISE_DURATION  1000

//
// Init
//
_OT_ProtocolV2 OT_ProtocolV2;
_OT_ProtocolV2::_OT_ProtocolV2() { }

void _OT_ProtocolV2::begin()
{
  this->characteristicCacheMutex = xSemaphoreCreateMutex();

  this->serviceUUID = BLEUUID(OT_SERVICEID);
  this->characteristicUUID = BLEUUID(OT_CHARACTERISTICID);

  log_i("Loading TempIDs from storage");
  if( TS_Storage.file_ids_readall(OT_TEMPID_MAX, tempIds) < OT_TEMPID_MAX)
  {
    log_w("Insufficient/error loading, creating TempIDs");
  }
  
  log_i("Loaded TempIDs");

  // DEBUG: populate characteristic cache once
  this->update_characteristic_cache();

  this->lastScanTs = 0;

  // Setup BLE and GATT profile
  BLEDevice::setMTU(OT_CR_MAXLEN);  // try to send whole message in 1 frame
  this->bleServer = TS_HAL.ble_server_get();
  //disable on connect/disconnect callbacks
  // this->bleServer->setCallbacks(this);
  this->bleService = bleServer->createService(this->serviceUUID);

  this->bleCharacteristic = bleService->createCharacteristic(this->characteristicUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  this->bleCharacteristic->setCallbacks(this);
  this->bleService->start();

  this->bleAdvertising = this->bleServer->getAdvertising();
  this->bleAdvertising->addServiceUUID(this->serviceUUID);
  this->bleAdvertising->setScanResponse(true);  // TODO: do we need this? apparently need this to be true.
  this->bleAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  // TODO: find out what setMin and setMax interval really means

}

void _OT_ProtocolV2::update()
{
  // don't turn off radio if we have connected clients
  uint16_t connectedCount = OT_ProtocolV2.get_connected_count();
  uint16_t sleepDuration = TS_HAL.random_get(1000, 3000);

  log_i("Devices connected: %d", connectedCount);
    
  if(connectedCount > 0)
  {
    TS_HAL.sleep(TS_SleepMode::Task, sleepDuration);
  }
  else
  {
    if(TS_POWER.get_state() == TS_PowerState::HIGH_POWER)
    {
      TS_HAL.sleep(TS_SleepMode::Task, sleepDuration);
    }
    else
    {
      // Power-saving sleep in low-power mode + no connected clients
      TS_HAL.sleep(TS_SleepMode::Light, sleepDuration);
    }
  }

  TS_DateTime datetime;
  TS_HAL.rtc_get(datetime);
  int secs = time_to_secs(&datetime);
  int secs_diff = time_diff(secs, lastScanTs, 86400);
  
  if(secs_diff >= SCAN_INTERVAL_SECS)
  {
    // spend up to 1s scanning, lowest acceptable rssi: -95
    OT_ProtocolV2.scan_and_connect(1, -95);

    lastScanTs = secs;
  }

  // enable advertising
  OT_ProtocolV2.advertising_start();
    
  // just advertise for 1s
  TS_HAL.sleep(TS_SleepMode::Task, ADVERTISE_DURATION);
    
  // disable advertising, get back to sleep
  OT_ProtocolV2.advertising_stop();
}

BLEUUID& _OT_ProtocolV2::getServiceUUID()
{
  return this->serviceUUID;
}

BLEUUID& _OT_ProtocolV2::getCharacteristicUUID()
{
  return this->characteristicUUID;
}

// gets the tempid by time from RTC, in seconds
// - quantize total seconds by 15 mins (900), mod by OT_TEMPID_MAX and return the relative TempID
OT_TempID& _OT_ProtocolV2::get_tempid_by_time(uint32_t seconds)
{
  return this->tempIds[(seconds / 900) % OT_TEMPID_MAX];
}

// sets the Nth tempid
// - returns false if failed
bool _OT_ProtocolV2::set_tempid(const OT_TempID &id, uint16_t n)
{
  if (n < 0 || n >= OT_TEMPID_MAX) return false;
  this->tempIds[n] = id;
  return true;
}

// Scans and do handshake w/ relevant peers
// - seconds: seconds to spend scanning
// - rssiCutoff: lowerbound rssi to ignore
bool _OT_ProtocolV2::scan_and_connect(uint8_t seconds, int8_t rssiCutoff)
{
  // Blocking scan
  BLEScanResults results = TS_HAL.ble_scan(seconds);

  uint16_t deviceCount = results.getCount();
  for (uint32_t i = 0; i < deviceCount; i++)
  {
    BLEAdvertisedDevice device = results.getDevice(i);

    // filter by service uuid
    // Note: getServiceUUID crashes for now, do not use
    if (!device.isAdvertisingService(this->serviceUUID)) continue;

    BLEAddress deviceAddress = device.getAddress();
    //uint8_t txPower = device.getTXPower();
    int8_t rssi = (int8_t)device.getRSSI();

    if(rssi < rssiCutoff) continue;

    log_i("%s rssi: %d", deviceAddress.toString().c_str(), rssi);

    // We only want to exchange if it has not been exchanged for more than some mins
    
    // Connect to each one and read + write iff parameters are correct
    // TODO: see if we can parallelize this process
    this->connect_and_exchange(device, deviceAddress, rssi);
  }
}

bool _OT_ProtocolV2::connect_and_exchange(BLEAdvertisedDevice &device, BLEAddress &address, int8_t rssi)
{
  BLEClient *bleClient = BLEDevice::createClient(); // new BLEClient
  bool ret = this->connect_and_exchange_impl(bleClient, device, address, rssi);

  // Trigger anyway to ensure cleanup, ble lib is glitchy
  bleClient->disconnect();

  // Waits up to 1s for it before forcefully deleting (and crashing)
  for(uint8_t stone = 0; stone < 10; ++stone)
  {
    if(bleClient->isConnected())
    {
      if(stone >= 9)
      {
        log_w("_OT_ProtocolV2::connect_and_exchange: bleClient still connected when deleted!");
      }
      
      TS_HAL.sleep(TS_SleepMode::Task, 100);
    }
  }
  
  delete bleClient;
  return ret;
}

bool _OT_ProtocolV2::connect_and_exchange_impl(BLEClient *bleClient, BLEAdvertisedDevice &device, BLEAddress &address, int8_t rssi)
{
  // Connect to the BLE Server

  // NOTE: used to be this but failed to do TraceStick-TraceStick connection
  // bleClient->connect(address, BLE_ADDR_TYPE_RANDOM);
  bleClient->connect(&device);
  
  BLERemoteService* pRemoteService;
  BLERemoteCharacteristic* pRemoteCharacteristic;

  if(!bleClient->isConnected()) 
  {
    log_w("Client connection failed");
    return false;
  }

  pRemoteService = bleClient->getService(this->serviceUUID);
  if (pRemoteService == NULL)
  {
    log_w("Failed to find our service UUID");
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(this->characteristicUUID);
  if (pRemoteCharacteristic == NULL)
  {
    log_w("Failed to find our characteristic UUID");
    return false;
  }

  if(!pRemoteCharacteristic->canRead() || !pRemoteCharacteristic->canWrite())
  {
    log_w("Unable to read or write");
    return false;
  }

  std::string buf;
  this->prepare_central_write_request(buf, rssi);
  pRemoteCharacteristic->writeValue(buf, false);
  
  log_i("BLE central Send: %s", buf.c_str());

  OT_ConnectionRecord connectionRecord;
  buf = pRemoteCharacteristic->readValue();
  
  if(!this->process_central_read_request(buf, connectionRecord))
  {
    log_w("Json parse error or read failed");
    return false;
  }

  log_i("BLE central Recv: %s", buf.c_str());
  
  // TODO: store data read into connectionRecord somewhere

  return true;
}

void _OT_ProtocolV2::advertising_start()
{
  this->bleAdvertising->start();
}

void _OT_ProtocolV2::advertising_stop()
{
  this->bleAdvertising->stop();
}

uint16_t _OT_ProtocolV2::get_connected_count()
{
  return this->bleServer->getConnectedCount();
}

void _OT_ProtocolV2::update_characteristic_cache()
{
  uint32_t secondsNow;
  TS_DateTime datetime;

  TS_HAL.rtc_get(datetime); // this takes some time
  secondsNow = datetime.second + datetime.minute * 60 + datetime.hour * 3600;

  OT_TempID& tempId = this->get_tempid_by_time( secondsNow );

  // Take semaphore to write string cache
  xSemaphoreTake( characteristicCacheMutex, portMAX_DELAY );
  this->charCacheTempId = tempId;
  this->prepare_peripheral_read_request( characteristicCache, tempId );
  xSemaphoreGive( characteristicCacheMutex );
}

void _OT_ProtocolV2::onConnect(BLEServer* pServer)
{
  log_i("Device connected to BLE");
}

void _OT_ProtocolV2::onDisconnect(BLEServer* pServer)
{
  log_i("Device disconnected from BLE");
}

// Callback after data is written from writer
void _OT_ProtocolV2::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string payload = pCharacteristic->getValue();
  if (payload.length() ==  0) return; // ignore empties

  log_i("BLE peripheral Recv: %s", payload.c_str());

  OT_ConnectionRecord cr;

  if(!this->process_peripheral_write_request(payload, cr))
  {
    log_w("Parse error or data invalid");
  }

  // TODO: cumulate to flash
}

// Callback before data is returned to reader
// - we got a chance to change characteristic data before data is returned
void _OT_ProtocolV2::onRead(BLECharacteristic* pCharacteristic)
{
  if (pCharacteristic != this->bleCharacteristic) return; // ignore things we don't care about

  // Take semaphore to read string
  xSemaphoreTake( characteristicCacheMutex, portMAX_DELAY );
  // write characteristicCache to characteristic
  pCharacteristic->setValue( characteristicCache );
  xSemaphoreGive( characteristicCacheMutex );

  log_i("BLE peripheral Send: %s", characteristicCache.c_str() );
}

//
// Serialization/deserialization
//

// Prepare crappy looking json payload
void _OT_ProtocolV2::prepare_peripheral_read_request(std::string &buf, std::string &id)
{
  buf = "{\"id\":\"";
  buf += id;
  buf += "\",\"v\":";
  buf += STRINGIFY( OT_PROTOVER );
  buf += ",\"o\":\"";
  buf += OT_ORG;
  buf += "\",\"mp\":\"";
  buf += DEVICE_NAME;
  buf += "\"}";
}

// Parse json into OT_ConnectionRecord
// - strict checking to avoid overflowing
bool _OT_ProtocolV2::process_peripheral_write_request(std::string& payload, OT_ConnectionRecord& connectionRecord)
{
  // enforce validity
  if (payload.length() > OT_CR_MAXLEN) return false; // filter excessive length

  StaticJsonDocument<OT_CR_MAXLEN> root;
  DeserializationError error = deserializeJson(root, payload);
  
  if (error)  // filter invalid json
  {
    log_e("%s", error.c_str());
    return false; 
  }
  
  if (!root.containsKey("id") || !root.containsKey("mc") || !root.containsKey("o") || !root.containsKey("rs") || !root.containsKey("v")) return false; // missing at least 1 key

  uint8_t ver = root["v"];
  if(ver != OT_PROTOVER) return false;  // filter invalid version

  connectionRecord.rssi = root["rs"];

  const char *raw = root["o"];
  if(raw == NULL) return false;
  connectionRecord.org = raw;
  if(connectionRecord.org.length() > OT_CR_SHORT_MAX) return false;

  raw = root["mc"];
  if(raw == NULL) return false;
  connectionRecord.deviceType = raw;
  if(connectionRecord.deviceType.length() > OT_CR_SHORT_MAX) return false;

  raw = root["id"];
  if(raw == NULL) return false;
  connectionRecord.id = raw;
  if(connectionRecord.deviceType.length() > OT_CR_ID_MAX) return false;

  return true;
}

// pack write request params into frame
// - id, mc, o, rs, v
void _OT_ProtocolV2::prepare_central_write_request(std::string& buf, int8_t rssi)
{
  char rssiBuf[5];
  
  buf = "{\"id\":\"";
  // Take semaphore to read cached tempid
  xSemaphoreTake( characteristicCacheMutex, portMAX_DELAY );
  buf += this->charCacheTempId;
  xSemaphoreGive( characteristicCacheMutex );
  buf += "\",\"v\":";
  buf += STRINGIFY( OT_PROTOVER );
  buf += ",\"o\":\"";
  buf += OT_ORG;
  buf += "\",\"mc\":\"";
  buf += DEVICE_NAME;
  buf += "\",\"rs\":";
  buf += itoa(rssi, rssiBuf, 10);
  buf += "}";
}

// Process frame into ConnectionRecord
bool _OT_ProtocolV2::process_central_read_request(std::string& payload, OT_ConnectionRecord& connectionRecord)
{
  // enforce validity
  if (payload.length() > OT_CR_MAXLEN) return false; // filter excessive length

  StaticJsonDocument<OT_CR_MAXLEN> root;
  DeserializationError error = deserializeJson(root, payload);

  connectionRecord.rssi = 127;  // unused field, set to known value
  
  if (error)  // filter invalid json
  {
    log_e("%s", error.c_str());
    return false; 
  }
  
  if (!root.containsKey("id") || !root.containsKey("mp") || !root.containsKey("o") || !root.containsKey("v")) return false; // missing at least 1 key

  uint8_t ver = root["v"];
  if(ver != OT_PROTOVER) return false;  // filter invalid version

  const char *raw = root["o"];
  if(raw == NULL) return false;
  connectionRecord.org = raw;
  if(connectionRecord.org.length() > OT_CR_SHORT_MAX) return false;

  raw = root["mp"];
  if(raw == NULL) return false;
  connectionRecord.deviceType = raw;
  if(connectionRecord.deviceType.length() > OT_CR_SHORT_MAX) return false;

  raw = root["id"];
  if(raw == NULL) return false;
  connectionRecord.id = raw;
  if(connectionRecord.deviceType.length() > OT_CR_ID_MAX) return false;

  return true;
}





