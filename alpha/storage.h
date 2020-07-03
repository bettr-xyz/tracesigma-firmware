//
// Persistent storage support for Flash based things such as
// - EEPROM
// --- RAM-backed mirror for working copy
// - Filesystem
// --- RAM-backed read cache, write-back cache

#ifndef __TS_STORAGE__
#define __TS_STORAGE__

#include "hal.h"

#include <SPIFFS.h>


//
// Classes
//

struct TS_Settings
{
  uint8_t settingsVersion;
  char userId[32];
  
  char wifiSsid[32];
  char wifiPass[32];
  bool upload_flag;
};



// Storage class
// - not threadsafe unless stated
class _TS_Storage
{
  public:
    _TS_Storage();

    void begin();

    //
    // Settings in Virtual EEPROM
    //
    TS_Settings*  settings_get();
    void          settings_reset();
    void          settings_load();
    void          settings_save();

    //
    // get free space %
    //
    uint8_t freespace_get_pct();
    uint8_t usedspace_get_pct();
    uint32_t freespace_get();
    uint32_t usedspace_get();

    //
    // File: ids
    // - strings are of uneven lengths, can either read-all or write-all

    // read all ids of maxCount, returns count of ids read
    uint8_t file_ids_readall(uint8_t maxCount, std::string *ids);

    // writeead all ids of maxCount, returns count of ids written
    uint8_t file_ids_writeall(uint8_t maxCount, std::string *ids);
    
  private:
    TS_Settings settingsRuntime;
};

extern _TS_Storage TS_Storage;

#endif
