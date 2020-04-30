//
// Bluetrace Opentrace V2, ESP32 C++ port
// From opentrace-android repo: https://github.com/opentrace-community/opentrace-android/tree/master/app/src/main/java/io/bluetrace/opentrace/protocol/v2
//
// Prefix trivial structures here with OT_
//

#ifndef __OPENTRACE_V2__
#define __OPENTRACE_V2__

#define OT_ORG      "SG_MOH"
#define OT_BUFSIZE  256
#define OT_PROTOVER 2

// TempID size = UID_SIZE + TIME_SIZE * 2
// Ref: https://github.com/opentrace-community/opentrace-cloud-functions/blob/master/functions/src/opentrace/getTempIDs.ts
#define OT_TEMPID_SIZE 29

#define OT_TEMPID_MAX 100

// TODO: store in mem as byte arrays instead of encoded strings

//
// Types
//

typedef OT_TempID byte[OT_TEMPID_SIZE];

//
// Structs
//


//
// Classes
//

class _OT_ProtocolV2
{
  private:
    //
    

  public:
    _OT_ProtocolV2();

    // quantize total seconds by 15 mins (900), mod by OT_TEMPID_MAX and return the relative TempID
    const OT_TempID& get_tempid(uint32_t seconds);

    // sets the Nth tempid
    void set_tempid(const OT_TempID &id, uint16_t n);

    // Pack read request params into frame : out buf
    bool prepare_peripheral_read_request(byte (&buf)[OT_BUFSIZE]);

    // Process frame into Connection Record : buf, addr
    bool process_peripheral_write_request(byte (&buf)[OT_BUFSIZE],
                                          String centralAddress);

    // pack write request params into frame : rssi, txPower, out buf
    bool prepare_central_write_request(uint8_t rssi,
                                       uint8_t txPower,
                                       byte (&buf)[OT_BUFSIZE]);

    // Process frame into ConnectionRecord
    bool process_central_read_request(byte (&buf)[OT_BUFSIZE],
                                      String peripheralAddress,
                                      uint8_t rssi,
                                      uint8_t txPower);

  
};

extern _OT_ProtocolV2 OT_ProtocolV2;

#endif
