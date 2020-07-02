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
};

struct TS_TempPeer
{
  TS_DateTime firstSeen;
  std::string org;
  std::string deviceType;
  uint8_t     mins;
  int8_t      rssi_min;
  int8_t      rssi_max;
  int16_t     rssi_sum;
  int8_t      rssi_samples;
  int16_t     rssi_dsquared;
};

struct TS_Peer
{
  uint16_t id;
  std::string org;
  std::string deviceType;

  // Incident
  uint8_t     hh, mm, ss;
  uint8_t     mins;
  int8_t      rssi_min;
  int8_t      rssi_max;
  int16_t     rssi_sum;
  int8_t      rssi_samples;
  int16_t     rssi_dsquared;
};

struct TS_PeerIterator
{
  
  // TODO
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
    // - TempIDs used for OTv2

    // read all ids of maxCount, returns count of ids read
    uint8_t file_ids_readall(uint8_t maxCount, std::string *ids);

    // write all ids of maxCount, returns count of ids written
    uint8_t file_ids_writeall(uint8_t maxCount, std::string *ids);

    // Log incident for OTv2 protocol
    bool peer_log_incident(std::string id, std::string org, std::string deviceType, int8_t rssi, TS_DateTime *current);

    // Obtain an iterator for reading through incidents
    // TODO
    TS_PeerIterator* peer_get_next_iterator(TS_PeerIterator* it);

    // Delete the last N days of incidents
    // TODO
    void peer_prune(uint8_t days, TS_DateTime *current);

    // Cleanup when necessary
    // Returns: entries removed
    uint16_t peer_cleanup(TS_DateTime *current);

    // Commits a specific key to flash
    bool peer_cache_commit(std::string key, TS_DateTime *current);
    
  private:
  
    TS_Settings settingsRuntime;

    // minutes since last cleanup
    uint8_t lastCleanupMins;

    // Peers which are < 5min
    std::map<std::string, TS_TempPeer> tempPeers;

    // Peers which are >= 5min
    // Data is cached here until flushed to file, could be 5 or 15 mins
    std::map<std::string, TS_Peer> peerCache;

};

extern _TS_Storage TS_Storage;

#endif
