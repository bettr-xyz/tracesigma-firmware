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

#define OT_ORG      "SG_MOH"
#define OT_BUFSIZE  256
#define OT_PROTOVER 2

#define OT_SERVICEID        "B82AB3FC-1595-4F6A-80F0-FE094CC218F9"
#define OT_CHARACTERISTICID "117BDD58-57CE-4E7A-8E87-7CCCDDA2A804"

// TempID size = UID_SIZE + TIME_SIZE * 2
// Ref: https://github.com/opentrace-community/opentrace-cloud-functions/blob/master/functions/src/opentrace/getTempIDs.ts
#define OT_TEMPID_SIZE 29
#define OT_TEMPID_MAX 100

// TODO: store in mem as byte arrays instead of encoded strings

//
// Types
//

typedef struct OT_TempID { uint8_t id[OT_TEMPID_SIZE]; } OT_TempID;

//
// Structs
//


//
// Classes
//

class _OT_ProtocolV2
: public BLECharacteristicCallbacks, public BLEServerCallbacks
{
  public:
    _OT_ProtocolV2();

    void begin();

    //////////
    // UUIDs
    BLEUUID& getServiceUUID();
    BLEUUID& getCharacteristicUUID();

    //////////
    // TempID

    // gets the tempid by time from RTC, in seconds
    const OT_TempID& get_tempid_by_time(uint32_t seconds);

    // sets the Nth tempid
    bool set_tempid(const OT_TempID &id, uint16_t n);

    

    //////////
    // Client scan and connect
    
    bool scan_and_connect(uint8_t seconds);


    // TODO: callback for storage


    //////////
    // Server advertise and listen
    void advertising_start();
    void advertising_stop();
    uint16_t get_connected_count();

    //////////
    // BLE callbacks
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    void onRead(BLECharacteristic* pCharacteristic) override;
    void onWrite(BLECharacteristic *pCharacteristic) override;

    //////////
    // Serialization/deserialization
    
    // Pack read request params into frame : out buf
    bool prepare_peripheral_read_request(uint8_t (&buf)[OT_BUFSIZE]);

    // Process frame into Connection Record : buf, addr
    bool process_peripheral_write_request(uint8_t (&buf)[OT_BUFSIZE],
                                          String centralAddress);

    // pack write request params into frame : rssi, txPower, out buf
    bool prepare_central_write_request(uint8_t rssi,
                                       uint8_t txPower,
                                       byte (&buf)[OT_BUFSIZE]);

    // Process frame into ConnectionRecord
    bool process_central_read_request(uint8_t (&buf)[OT_BUFSIZE],
                                      String peripheralAddress,
                                      uint8_t rssi,
                                      uint8_t txPower);

  private:
    // Store OT_TEMPID_MAX TempIDs for rotation
    OT_TempID tempIds[OT_TEMPID_MAX];

    BLEUUID   serviceUUID;
    BLEUUID   characteristicUUID;
    
    BLEServer         *bleServer;
    BLEService        *bleService;
    BLECharacteristic *bleCharacteristic;
    BLEAdvertising    *bleAdvertising;

    // NOTE: no need for synchronization as callbacks writing to this are from the same worker
    uint16_t          bleConnectedClients;
};

extern _OT_ProtocolV2 OT_ProtocolV2;

#endif
