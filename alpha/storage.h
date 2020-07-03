//
// Persistent storage support for Flash based things such as
// - EEPROM
// - Filesystem

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

struct TS_Peer
{
  uint16_t id;  // may not be used until just about to store
  
  std::string org;
  std::string deviceType;

  // Incident
  TS_DateTime firstSeen;
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

    //
    // Peering functions
    // 

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

    // Commits a specific key to flash and removes entry from peerCache
    void peer_cache_commit(const std::string &key, TS_Peer *peer, TS_DateTime *current);

    // Commit all entries to flash, useful when gracefully shutting down
    void peer_cache_commit_all(TS_DateTime *current);
    
  private:
  
    TS_Settings settingsRuntime;

    // minutes since last cleanup
    uint8_t lastCleanupMins;

    // Incident peers which are < 5min
    std::map<std::string, TS_Peer> tempPeers;

    // Peers which are >= 5min
    // Data is cached here until flushed to file, could be 5 - 18 mins thereabouts
    std::map<std::string, TS_Peer> peerCache;

    //
    // Peer file functions
    //

    // Get an existing or add new peer id
    // - typically only used if entry is not in peerCache
    uint16_t peer_id_get_or_add(const std::string &tempId, TS_Peer *peer);

    // Append incident to peer incident file
    void peer_incident_add(TS_Peer *peer);
};

extern _TS_Storage TS_Storage;

#endif
