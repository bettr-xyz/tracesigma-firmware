//
// Persistent storage support for Flash based things such as
// - EEPROM
// - Filesystem

#ifndef __TS_STORAGE__
#define __TS_STORAGE__

#include "hal.h"
#include "tests.h"
#include <SPIFFS.h>
#include <list>

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
  TS_Peer();
  
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

class _TS_Storage;
class TS_PeerIterator
{
  friend class _TS_Storage;
  
  public:
    TS_PeerIterator();
    virtual ~TS_PeerIterator();

    std::string *getPeerId();
    TS_Peer *getPeerIncident();
    
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

    // Obtain an iterator to get next day
    // - delete after use
    TS_PeerIterator* peer_get_next(TS_PeerIterator* it);
    TS_PeerIterator* peer_get_next_peer(TS_PeerIterator* it);     // Get next peer
    TS_PeerIterator* peer_get_next_incident(TS_PeerIterator* it); // Get next incident of current peer

    // Delete the last N days of incidents depending on free space
    void peer_prune(uint8_t days, TS_DateTime *current);

    // Cleanup when necessary
    // - ideally run once before logging peers
    // - run at least once a minute
    // Returns: number of entries removed
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

    //
    // Other helper functions
    //
    
    void peer_rssi_add_sample(TS_Peer *peer, int8_t rssi);
};

extern _TS_Storage TS_Storage;



//
// Tests
//

#ifdef TESTDRIVER
static class _TS_StorageTests : public _TS_Tests
{  
public:
  std::string test_id;
  std::string test_org;
  std::string test_device;
  TS_DateTime test_time;
  int8_t test_rssi;

  bool test_peer_log_pass;

  void init() override
  {
    test_id = "abc";
    test_org = "test org";
    test_device = "test device";
    test_time.day = 6;
    test_time.month = 6;
    test_time.year = 2020;
    test_time.hour = 10;
    test_time.minute = 10;
    test_time.second = 10;
    test_rssi = -30;

    test_peer_log_pass = false;
  }

  void setup() override {}

  void teardown() override {}

  // test writing a log
  bool test_peer_log()
  { 
    test_peer_log_pass = false;
    if( !TS_Storage.peer_log_incident( test_id, test_org, test_device, test_rssi, &test_time ) )
    {
      log_e("peer_log_incident expected to return true");
      return false;
    }

    test_peer_log_pass = true;
    return true;
  }
  
  // TODO: test iterate
  bool test_iterate_logs()
  {
    if(!test_peer_log_pass)
    {
      log_e("Test depends on test_peer_log passing");
      return false;
    }

    // TODO:
    return false;
  }

  // TODO: test cleanup
  bool test_cleanup_before_elapsed()
  {

    // TODO:
    return false;
  }

  bool test_cleanup_after_elapsed()
  {
    // TODO:
    return false;
  }

  // Ctor
  _TS_StorageTests()
  { 
    add(std::bind(&_TS_StorageTests::test_peer_log, this), "test_peer_log");
    add(std::bind(&_TS_StorageTests::test_iterate_logs, this), "test_iterate_logs");
    add(std::bind(&_TS_StorageTests::test_cleanup_before_elapsed, this), "test_cleanup_before_elapsed");
    add(std::bind(&_TS_StorageTests::test_cleanup_after_elapsed, this), "test_cleanup_after_elapsed");
  }
} TS_StorageTests;

#endif
// end TESTDRIVER

#endif
