//
// Persistent storage support for Flash based things such as
// - EEPROM
// - Filesystem

#ifndef __TS_STORAGE__
#define __TS_STORAGE__

#include "hal.h"
#include "tests.h"
#include <list>
#include "FFat.h"

//
// Classes
//

// Class pre-def
class _TS_StorageTests;
class _TS_Storage;

struct TS_Settings
{
  uint8_t settingsVersion;
  char userId[32];
  
  char wifiSsid[32];
  char wifiPass[32];
  bool upload_flag;
};

typedef struct TS_MacAddress { uint8_t b[6]; } TS_MacAddress;

struct TS_Peer
{
  TS_Peer();
  
  uint16_t id;  // may not be used until just about to store

  TS_MacAddress deviceMac;
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

class TS_PeerIterator
{
  friend class _TS_Storage;
  
  public:
    TS_PeerIterator();
    virtual ~TS_PeerIterator();

    std::string *getDayFile();
    std::string *getPeerId();
    TS_Peer *getPeerIncident();

    uint8_t log();
    
  protected:
    std::list<std::string> dayFileNames;
    std::string dayFileName;
    
    File fileId;
    File fileIncident;

    bool validPeer;
    std::string peerId;

    bool validIncident;
    TS_Peer peer;
};



// Storage class
// - not threadsafe unless stated
class _TS_Storage
{
  friend class _TS_StorageTests;
  
  public:
    _TS_Storage();

    void begin();
    void reset();

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
    bool peer_log_incident(std::string &id, std::string &org, std::string &deviceType, int8_t rssi, TS_DateTime *current, TS_MacAddress &deviceMac);

    // Obtain an iterator to get next day
    // - delete after use
    TS_PeerIterator* peer_get_next(TS_PeerIterator* it);
    TS_PeerIterator* peer_get_next_peer(TS_PeerIterator* it);     // Get next peer
    TS_PeerIterator* peer_get_next_incident(TS_PeerIterator* it); // Get next incident of current peer

    // Delete the last N days of incidents depending on free space
    int peer_prune(uint8_t days, TS_DateTime *current);

    // Cleanup when necessary
    // - ideally run once before logging peers
    // - run at least once a minute
    // Returns: number of entries removed
    uint16_t peer_cleanup(TS_DateTime *current);

    // Commits a specific key to flash and removes entry from peerCache
    void peer_cache_commit(const std::string &key, TS_Peer *peer, TS_DateTime *current);

    // Commit all entries to flash, useful when gracefully shutting down
    int peer_cache_commit_all(TS_DateTime *current);
    
  private:
  
    TS_Settings settingsRuntime;

    // minutes since last cleanup
    uint8_t lastCleanupMins;

    // Incident peers which are < 5min
    std::map<std::string, TS_Peer> tempPeers;

    // Peers which are >= 5min
    // Data is cached here until flushed to file, could be 5 - 18 mins thereabouts
    std::map<std::string, TS_Peer> peerCache;

    // Map of MAC addresses to peer structs
    std::map<TS_MacAddress, TS_Peer*> peersMac;

    //
    // Peer file functions
    //

    // Get an existing or add new peer id
    // - typically only used if entry is not in peerCache
    uint16_t peer_id_get_or_add(const std::string &tempId, TS_Peer *peer);

    // Append incident to peer incident file
    void peer_incident_add(TS_Peer *peer);

    //
    // Other helper functions
    //
    
    void peer_rssi_add_sample(TS_Peer *peer, int8_t rssi);

    bool filename_older_than(const char * filename, int8_t days, TS_DateTime *current);

    void set_default_settings();

    // tempPeers / peerCache modification functions

    void erase_temppeer_by_tempid(const std::string &tempId);
    void erase_peercache_by_tempid(const std::string &tempId);
    void move_temppeer_to_peercache(std::map<std::string, TS_Peer>::iterator &peer);
    void create_temppeer(const std::string &tempId, const TS_Peer &peer);
};

extern _TS_Storage TS_Storage;

#endif
