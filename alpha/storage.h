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

// Storage class
// - not threadsafe unless stated
class _TS_Storage
{
  public:
    _TS_Storage();

    void begin();

    // get free space %
    uint8_t get_freespace_pct();
    uint8_t get_usedspace_pct();

    //
    // File: ids
    // - strings are of uneven lengths, can either read-all or write-all

    // read all ids of maxCount, returns count of ids read
    template<size_t N>
    uint8_t file_ids_readall(uint8_t maxCount, std::string (&id)[N]);

    // writeead all ids of maxCount, returns count of ids written
    template<size_t N>
    void file_ids_writeall(uint8_t maxCount, std::string (&id)[N]);
};

extern _TS_Storage TS_Storage;

#endif
