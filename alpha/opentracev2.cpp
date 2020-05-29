#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <ArduinoJson.h>

#include "opentracev2.h"
#include "hal.h"
#include "cleanbox.h"

// For Base64 encode
extern "C" {
#include "crypto/base64.h"
}
// TODO:
// unsigned char * encoded = base64_encode((const unsigned char *)toEncode, strlen(toEncode), &outputLength);
// unsigned char * decoded = base64_decode((const unsigned char *)toDecode, strlen(toDecode), &outputLength);

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

  // DEBUG: temporarily fill tempIds with predictable fluff
  for (int i = 0; i < OT_TEMPID_MAX; ++i)
  {
    this->tempIds[i] = "8Vej+n4NAutyZlS1ItKDL//RcfqWP/Tq/T/BBBUOsmAF0U+TGBqd2xcMhpfcSOyN1cSGN3znSGguodP+NQ==";
  }

  // DEBUG: populate characteristic cache once
  this->update_characteristic_cache();


  // Setup BLE and GATT profile
  BLEDevice::setMTU(OT_CR_MAXLEN);  // try to send whole message in 1 frame
  this->bleServer = TS_HAL.ble_server_get();
  this->bleServer->setCallbacks(this);
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

    Serial.print(deviceAddress.toString().c_str());
    Serial.print(" rssi: ");
    Serial.println(rssi);

    // We do not need a map of last-seen as this function should have long intervals between calls
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
        Serial.println("WARN: _OT_ProtocolV2::connect_and_exchange: bleClient still connected when deleted!");
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
    Serial.println(F("Client connection failed"));
    return false;
  }

  pRemoteService = bleClient->getService(this->serviceUUID);
  if (pRemoteService == NULL)
  {
    Serial.println(F("Failed to find our service UUID"));
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(this->characteristicUUID);
  if (pRemoteCharacteristic == NULL)
  {
    Serial.println(F("Failed to find our characteristic UUID"));
    return false;
  }

  if(!pRemoteCharacteristic->canRead() || !pRemoteCharacteristic->canWrite())
  {
    Serial.println(F("Unable to read or write"));
    return false;
  }

  std::string buf;
  this->prepare_central_write_request(buf, rssi);
  pRemoteCharacteristic->writeValue(buf, false);
  Serial.print("BLE central Send: ");
  Serial.println(buf.c_str());

  OT_ConnectionRecord connectionRecord;
  buf = pRemoteCharacteristic->readValue();
  
  if(!this->process_central_read_request(buf, connectionRecord))
  {
    Serial.println(F("Json parse error or read failed"));
    return false;
  }

  Serial.print("BLE central Recv: ");
  Serial.println(buf.c_str());
  
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
  Serial.println("Device connected to BLE");
}

void _OT_ProtocolV2::onDisconnect(BLEServer* pServer)
{
  Serial.println("Device disconnected from BLE");
}

// Callback after data is written from writer
void _OT_ProtocolV2::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string payload = pCharacteristic->getValue();
  if (payload.length() ==  0) return; // ignore empties

  Serial.print("BLE peripheral Recv: ");
  Serial.println(payload.c_str());

  OT_ConnectionRecord cr;

  if(!this->process_peripheral_write_request(payload, cr))
  {
    Serial.println("Parse error or data invalid");
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

  Serial.print("BLE peripheral Send: ");
  Serial.println( characteristicCache.c_str() );
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
    Serial.println(error.c_str());
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
    Serial.println(error.c_str());
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





