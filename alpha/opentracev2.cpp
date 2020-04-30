#include "opentracev2.h"
#include "hal.h"

// For Base64 encode
extern "C" {
#include "crypto/base64.h"
}
// TODO: unsigned char * encoded = base64_encode((const unsigned char *)toEncode, strlen(toEncode), &outputLength);

//
// Internal vars
//

// Store OT_TEMPID_MAX TempIDs for rotation
OT_TempID tempIds[OT_TEMPID_MAX];

//
// Init
//
_OT_ProtocolV2 OT_ProtocolV2;
_OT_ProtocolV2::_OT_ProtocolV2()
{
  // DEBUG: temporarily fill tempIds with predictable fluff
  for(int i = 0; i < OT_TEMPID_MAX; ++i)
  {
    memset(tempIds[i], i, OT_TEMPID_SIZE);   
  }
}






