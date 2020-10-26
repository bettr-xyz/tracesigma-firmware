//
// Bluetrace Opentrace V2, ESP32 C++ port
// From opentrace-android repo: https://github.com/opentrace-community/opentrace-android/tree/master/app/src/main/java/io/bluetrace/opentrace/protocol/v2
//
// Prefix trivial structures here with OT_
//

#ifndef __OPENTRACE_V2__
#define __OPENTRACE_V2__

#include <stdint.h>
#include <Arduino.h>
#include <BLEUUID.h>
#include <BLECharacteristic.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEDevice.h>

#include "storage.h"

#define OT_ORG          "SG_MOH"
#define OT_PROTOVER     2

#define OT_CR_MAXLEN    256
#define OT_CR_ID_MAX    128
#define OT_CR_SHORT_MAX  32

#define OT_SERVICEID        "B82AB3FC-1595-4F6A-80F0-FE094CC218F9"
#define OT_CHARACTERISTICID "117BDD58-57CE-4E7A-8E87-7CCCDDA2A804"

#define OT_TEMPID_MAX   100

// TODO: store in mem as byte arrays instead of encoded strings

//
// Types
//

typedef std::string OT_TempID;

//
// Structs
//

// Connection record
struct OT_ConnectionRecord
{
  std::string id;
  std::string org;
  std::string deviceType;
  int8_t      rssi;   // valid range: -128 to 127
};

//
// Classes
//

class _OT_ProtocolV2
: public BLECharacteristicCallbacks, public BLEServerCallbacks
{
  public:
    _OT_ProtocolV2();

    void begin();

    // Blocking update, call at least once a second
    void update();

    //////////
    // UUIDs
    BLEUUID& getServiceUUID();
    BLEUUID& getCharacteristicUUID();

    //////////
    // TempID

    // gets the tempid by time from RTC, in seconds
    OT_TempID& get_tempid_by_time(uint32_t seconds);

    // sets the Nth tempid
    bool set_tempid(const OT_TempID &id, uint16_t n);

    

    //////////
    // Client scan and connect
    
    bool scan_and_connect(uint8_t seconds, int8_t rssiCutoff);

    bool connect_and_exchange(BLEAdvertisedDevice &device, BLEAddress &address, int8_t rssi);
    bool connect_and_exchange_impl(BLEClient *bleClient, BLEAdvertisedDevice &device, BLEAddress &address, int8_t rssi);

    // TODO: callback for storage


    //////////
    // Server advertise and listen
    void advertising_start();
    void advertising_stop();
    uint16_t get_connected_count();

    void update_characteristic_cache();

private:

    //////////
    // BLE callbacks
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    void onWrite(BLECharacteristic *pCharacteristic) override;
    void onRead(BLECharacteristic* pCharacteristic) override;
    
    //////////
    // Serialization/deserialization
    
    // Pack read request params into frame
    void prepare_peripheral_read_request(std::string& buf,
                                         std::string &id);

    // Process frame into Connection Record
    bool process_peripheral_write_request(std::string& payload,
                                          OT_ConnectionRecord& connectionRecord);

    // pack write request params into frame
    void prepare_central_write_request(std::string& buf,
                                       int8_t rssi);

    // Process frame into ConnectionRecord
    bool process_central_read_request(std::string& payload,
                                      OT_ConnectionRecord& connectionRecord);

    ////////
    // Helpers

    void get_mac_from_ble_address(BLEAddress &fromBleAddress, TS_MacAddress &toMacAddress);

  private:
    // Store OT_TEMPID_MAX TempIDs for rotation
    // - Average TempID : ~86 chars of base64 encode, 8600b total
    // TODO: this should be stored on flash at some point
    OT_TempID tempIds[OT_TEMPID_MAX];

    BLEUUID   serviceUUID;
    BLEUUID   characteristicUUID;
    
    BLEServer         *bleServer;
    BLEService        *bleService;
    BLECharacteristic *bleCharacteristic;
    BLEAdvertising    *bleAdvertising;

    // Caches the json string to be put in characteristic cache
    // - we do not want to repeatedly recompute this string, as its only updated once every 15 min
    std::string       characteristicCache;
    OT_TempID         charCacheTempId;
    SemaphoreHandle_t characteristicCacheMutex;

    int               lastScanTs;
};

extern _OT_ProtocolV2 OT_ProtocolV2;

#endif
