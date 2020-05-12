#include "storage.h"
#include "hal.h"

_TS_Storage TS_Storage;
_TS_Storage::_TS_Storage() { }

void _TS_Storage::begin()
{
  // format on error, basepath, maxOpenFiles
  // - use the shortest path, f - spi flash
  if(!SPIFFS.begin(true, "/f", 1))
  {
    TS_HAL.log("SPIFFS init error");
  }
}

uint8_t _TS_Storage::get_freespace_pct()
{
  return 100 - this->get_usedspace_pct();
}

uint8_t _TS_Storage::get_usedspace_pct()
{
  uint32_t total = SPIFFS.totalBytes();
  uint32_t used = SPIFFS.usedBytes();
  uint32_t pct = (used*100)/total;  // 0-100 range
  return (uint8_t)pct;
}

uint32_t _TS_Storage::get_freespace()
{
  uint32_t total = SPIFFS.totalBytes();
  return total - SPIFFS.usedBytes();
}

uint32_t _TS_Storage::get_usedspace()
{
  return SPIFFS.usedBytes();
}

uint8_t _TS_Storage::file_ids_readall(uint8_t maxCount, std::string *ids)
{
  // TODO: an optimization is to read only a lookup table, not all the contents
  
  uint8_t count = 0;
  File f = SPIFFS.open("/ids", "r+");
  if(!f) return 0;
  
  for(uint8_t i = 0; i < maxCount; ++i)
  {
    if(!f.available()) break;
    String s = f.readStringUntil(',');
    ids[count] = std::string(s.c_str());
    ++count;
  }

  f.close();
  return count;
}

uint8_t _TS_Storage::file_ids_writeall(uint8_t maxCount, std::string *ids)
{
  // TODO: test free space and run cleanup
  
  File f = SPIFFS.open("/ids", "w+");
  if(!f) return 0;

  for(uint8_t i = 0; i < maxCount; ++i)
  {
    f.print(ids[i].c_str());
    f.print(',');
  }

  f.close();
  return maxCount;
}

