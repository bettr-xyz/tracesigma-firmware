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
// Internal vars
//

// Store OT_TEMPID_MAX TempIDs for rotation
OT_TempID tempIds[OT_TEMPID_MAX];
BLEUUID   serviceUUID;
BLEUUID   characteristicUUID;

//
// Init
//
_OT_ProtocolV2 OT_ProtocolV2;
_OT_ProtocolV2::_OT_ProtocolV2()
{
  serviceUUID = BLEUUID(OT_SERVICEID);
  characteristicUUID = BLEUUID(OT_CHARACTERISTICID);
  
  // DEBUG: temporarily fill tempIds with predictable fluff
  for(int i = 0; i < OT_TEMPID_MAX; ++i)
  {
    memset(&tempIds[i], i, OT_TEMPID_SIZE);
  }
}

BLEUUID& _OT_ProtocolV2::getServiceUUID()
{
  return serviceUUID;
}

BLEUUID& _OT_ProtocolV2::getCharacteristicUUID()
{
  return characteristicUUID;  
}

// gets the tempid by time from RTC, in seconds
// - quantize total seconds by 15 mins (900), mod by OT_TEMPID_MAX and return the relative TempID
const OT_TempID& _OT_ProtocolV2::get_tempid_by_time(uint32_t seconds)
{
  return tempIds[(seconds / 900) % OT_TEMPID_MAX];
}

// sets the Nth tempid
// - returns false if failed
bool _OT_ProtocolV2::set_tempid(const OT_TempID &id, uint16_t n)
{
  if (n < 0 || n >= OT_TEMPID_MAX) return false;
  tempIds[n] = id;
  return true;
}

