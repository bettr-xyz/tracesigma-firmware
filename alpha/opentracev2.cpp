#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "opentracev2.h"
#include "hal.h"

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
  this->serviceUUID = BLEUUID(OT_SERVICEID);
  this->characteristicUUID = BLEUUID(OT_CHARACTERISTICID);
  
  // DEBUG: temporarily fill tempIds with predictable fluff
  for(int i = 0; i < OT_TEMPID_MAX; ++i)
  {
    memset(&this->tempIds[i], i, OT_TEMPID_SIZE);
  }

  // Setup BLE and GATT profile 
  this->bleServer = TS_HAL.ble_server_get();
  this->bleServer->setCallbacks(this);
  this->bleService = bleServer->createService(serviceUUID);
  
  this->bleCharacteristic = bleService->createCharacteristic(characteristicUUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  this->bleCharacteristic->setCallbacks(this);
  this->bleService->start();

  this->bleAdvertising = bleServer->getAdvertising();
  this->bleAdvertising->addServiceUUID(serviceUUID);
  this->bleAdvertising->setScanResponse(true);  // TODO: do we need this? 
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
const OT_TempID& _OT_ProtocolV2::get_tempid_by_time(uint32_t seconds)
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

bool _OT_ProtocolV2::scan_and_connect(uint8_t seconds)
{
    // Blocking scan
    BLEScanResults results = TS_HAL.ble_scan(seconds);
    
    uint16_t deviceCount = results.getCount();
    for (uint32_t i = 0; i < deviceCount; i++)
    {
      BLEAdvertisedDevice device = results.getDevice(i);

      // filter by service uuid
      // Note: getServiceUUID crashes for now, do not use
      if(!device.isAdvertisingService(this->serviceUUID)) continue;

      BLEAddress deviceAddress = device.getAddress();
      uint8_t txPower = device.getTXPower();
      int rssi = device.getRSSI();

      Serial.println(deviceAddress.toString().c_str());
      Serial.println(txPower);
      Serial.println(rssi);

      // TODO: connect to each one and send data
    }
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
  return this->bleConnectedClients;
}

void _OT_ProtocolV2::onConnect(BLEServer* pServer)
{
  Serial.println("Device connected to BLE");
  ++bleConnectedClients;
}

void _OT_ProtocolV2::onDisconnect(BLEServer* pServer)
{
  Serial.println("Device disconnected from BLE");
  --bleConnectedClients;
}

void _OT_ProtocolV2::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string rxValue = pCharacteristic->getValue();

  if (rxValue.length() > 0) {
    Serial.print("BLE Received Value: ");
    for (int i = 0; i < rxValue.length(); i++)
      Serial.print(rxValue[i]);
    Serial.println();
  }
}

void _OT_ProtocolV2::onRead(BLECharacteristic* pCharacteristic)
{
  Serial.println("Device read request on");
  Serial.println(pCharacteristic->toString().c_str());
}


// TODO:  serializers and deserializers







