#include "storage.h"
#include "hal.h"

_TS_Storage TS_Storage;
_TS_Storage::_TS_Storage() { }

void _TS_Storage::begin()
{
  // format on error, basepath, maxOpenFiles
  // - use the shortest path, f - spi flash
  SPIFFS.begin(true, "/f", 1);  
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

template<size_t N>
uint8_t _TS_Storage::file_ids_readall(uint8_t maxCount, std::string (&id)[N])
{
  // TODO: an optimization is to read only a lookup table, not all the contents
  
  uint8_t count = 0;
  File f = SPIFFS.open("ids", "r+");
  if(!f) return 0;
  
  for(uint8_t i = 0; i < maxCount; ++i)
  {
    if(!f.available()) break;
    String s = f.readStringUntil(',');
    id[count] = std::string(s.c_str());
    ++count;
  }

  f.close();
  return count;
}

template<size_t N>
void _TS_Storage::file_ids_writeall(uint8_t maxCount, std::string (&id)[N])
{
  // TODO: test free space and run cleanup
  
  File f = SPIFFS.open("ids", "w+");
  if(!f) return 0;

  for(uint8_t i = 0; i < maxCount; ++i)
  {
    f.print(id[i]);
    f.print(',');
  }

  f.close();
}

