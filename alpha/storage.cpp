#include "storage.h"
#include "hal.h"
#include "cleanbox.h"
#include "FS.h"
#include <EEPROM.h>
#include "storage_ffat.h"

#define EEPROM_SIZE       1024
#define SETTINGS_VERSION  0x02

// Cleanup every 3 mins
#define CLEANUP_MINS      3

#define TEMPPEERS_MAX     100
#define PEERCACHE_MAX     30

#define PEERCACHE_ACCEPT_MINS 5

#define TEMPPEERS_MAXAGE  420    // 7 min
#define PEERCACHE_MAXAGE  1800   // 30 min

#define PEER_MAXIDLE      240    // 4 min

#define SECS_PER_DAY      86400
#define SECS_PER_MIN      60

#ifndef DEFAULT_UID
  #define DEFAULT_UID "0123456789"
#endif
#ifndef WIFI_SSID
  #define WIFI_SSID "test"        // Enter your SSID here
#endif
#ifndef WIFI_PASS
  #define WIFI_PASS "password"    // Enter your WiFi password here
#endif





struct PeerIncidentFileFrame
{
  TS_DateTime firstSeen;
  uint8_t     mins;
  int8_t      rssi_min;
  int8_t      rssi_max;
  int16_t     rssi_sum;
  int8_t      rssi_samples;
  int16_t     rssi_dsquared;
};

//
// TS_Peer
//

TS_Peer::TS_Peer()
: id(0), mins(0), rssi_min(0), rssi_max(0), rssi_sum(0), rssi_samples(0), rssi_dsquared(0)
{
}

//
// TS_PeerIterator
//

TS_PeerIterator::TS_PeerIterator()
: validPeer(false), validIncident(false)
{ 
}

TS_PeerIterator::~TS_PeerIterator()
{
  if(fileId)
  {
    fileId.close();
  }

  if(fileIncident)
  {
    fileIncident.close();
  }
}

std::string * TS_PeerIterator::getDayFile()
{
  return &this->dayFileName;
}

std::string * TS_PeerIterator::getPeerId()
{
  if(!validPeer) return NULL;
  return &this->peerId;
}

TS_Peer * TS_PeerIterator::getPeerIncident()
{
  if(!validIncident) return NULL;
  return &this->peer;
}

uint8_t TS_PeerIterator::log()
{
  std::string *peerId = this->getPeerId();
  TS_Peer *pi = this->getPeerIncident();

  if(peerId == NULL)
  {
    log_i("TS_PeerIterator %s None", this->getDayFile()->c_str());
    return 0;
  }
  else if(pi == NULL)
  {
    log_i("TS_PeerIterator %s %s None ", this->getDayFile()->c_str(), peerId->c_str());
    return 1;
  }
  else
  {
    log_i("TS_PeerIterator %s %s %d %s %s (%d-%d-%d %d:%d:%d) (%d) %d, %d, %d %d %d",
      this->getDayFile()->c_str(), peerId->c_str(), pi->id, pi->org.c_str(), pi->deviceType.c_str(),
      pi->firstSeen.day, pi->firstSeen.month, pi->firstSeen.year, pi->firstSeen.hour, pi->firstSeen.minute, pi->firstSeen.second,
      pi->mins, pi->rssi_min, pi->rssi_max, pi->rssi_sum, pi->rssi_samples, pi->rssi_dsquared);
    return 2;
  }
}


//
// TS_Storage
//

_TS_Storage TS_Storage;
_TS_Storage::_TS_Storage()
{
  reset();
}

void _TS_Storage::begin()
{
  EEPROM.begin(EEPROM_SIZE);

  // Initialize Settings, copy from EEPROM and possibly reinitialize
  if(sizeof(this->settingsRuntime) > EEPROM_SIZE)
  {
    TS_HAL.fail_reboot("FATAL: TS_Settings larger than EEPROM_SIZE!");
  }

  this->settings_load();
  if(this->settingsRuntime.settingsVersion != SETTINGS_VERSION)
  {
    this->settings_reset();
  }

  // Initialize FFat
  // format on error
  if(!FFat.begin())
  {
    log_e("FFat Mount Failed, attempting format and remount... ");

    FFat.format();

    if(!FFat.begin())
    {
      log_e("FFat ReMount Failed.");
      return;
    }
  }    

  // dump all files
  {
    /*log_i("Listing all files");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file)
    {
      log_i("FILE: %s", file.name());
      file.close();
      file = root.openNextFile();
    }
    
    root.close();*/
    StorageFFat::listDir("/", 4);
  }

  this->set_default_settings();
}

void _TS_Storage::reset()
{
  // Reset data structures
  lastCleanupMins = 0;
  tempPeers.clear();
  peerCache.clear();
}



//
// Settings in Virtual EEPROM
//

TS_Settings* _TS_Storage::settings_get()
{
  return &settingsRuntime;
}

void _TS_Storage::settings_reset()
{
  // Reset values to defaults
  this->settingsRuntime.settingsVersion = SETTINGS_VERSION;
  memset(this->settingsRuntime.userId, 0, sizeof(this->settingsRuntime.userId));
  memset(this->settingsRuntime.wifiSsid, 0, sizeof(this->settingsRuntime.wifiSsid));
  memset(this->settingsRuntime.wifiPass, 0, sizeof(this->settingsRuntime.wifiPass));
  this->settings_save();
}

void _TS_Storage::settings_load()
{
  EEPROM.readBytes(0, &(this->settingsRuntime), sizeof(this->settingsRuntime));
}

void _TS_Storage::settings_save()
{
  EEPROM.writeBytes(0, &(this->settingsRuntime), sizeof(this->settingsRuntime));
  EEPROM.commit();
}



//
// get free space %
//
uint8_t _TS_Storage::freespace_get_pct()
{
  return 100 - this->usedspace_get_pct();
}

uint8_t _TS_Storage::usedspace_get_pct()
{
  uint32_t totalBytes = StorageFFat::totalBytes();
  uint32_t freeBytes = StorageFFat::freeBytes();
  uint32_t pct = ((totalBytes - freeBytes) * 100) / totalBytes;  // 0-100 range
  return (uint8_t)pct;
}

uint32_t _TS_Storage::freespace_get()
{
  uint32_t total = StorageFFat::freeBytes();
}

uint32_t _TS_Storage::usedspace_get()
{
  uint32_t totalBytes = StorageFFat::totalBytes();
  uint32_t freeBytes = StorageFFat::freeBytes();
  return totalBytes - freeBytes;
}

//
// File: ids
//

uint8_t _TS_Storage::file_ids_readall(uint8_t maxCount, std::string *ids)
{
  // TODO: an optimization is to read only a lookup table, not all the contents
  
  uint8_t count = 0;
  File f = StorageFFat::openRead("/ids");
  if(!f)
  {
    log_e("Failed to open ids for read");
    return 0;
  }
  
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
  
  File f = StorageFFat::openWrite("/ids");
  if(!f)
  {
    log_e("Failed to open file /ids for w+");
    return 0;
  }

  for(uint8_t i = 0; i < maxCount; ++i)
  {
    f.print(ids[i].c_str());
    f.print(',');
  }

  f.close();
  return maxCount;
}

//
// Peering functions
// 

bool _TS_Storage::peer_log_incident(std::string id, std::string org, std::string deviceType, int8_t rssi, TS_DateTime *current)
{
  int currentTimeToSecs = time_to_secs(current);
  
  // if exist in tempPeers, cumulate. If mins >= PEERCACHE_ACCEPT_MINS , move into peerCache, delete from tempPeers
  {
    auto peer = this->tempPeers.find(id);
    if (peer != this->tempPeers.end())
    {
      // found, cumulate rssi samples
      peer_rssi_add_sample(&peer->second, rssi);
  
      // update nearest minutes
      int peerTimeDiff = time_diff(currentTimeToSecs, time_to_secs(&peer->second.firstSeen), SECS_PER_DAY);
      peer->second.mins = peerTimeDiff / 60;
  
      if(peer->second.mins >= PEERCACHE_ACCEPT_MINS)
      {
        // Get/add an id
        peer->second.id = this->peer_id_get_or_add( peer->first, &peer->second );
        if(peer->second.id == 0)
        {
          log_e("Unable to get next id from peer id file");
          return false;
        }
        
        // Move entry to peercache
        this->peerCache[ peer->first ] = peer->second;
        this->tempPeers.erase(peer);
      }
      
      return true;
    }
  }

  // find in peercache
  {
    auto peer = this->peerCache.find(id);
    if (peer != this->peerCache.end())
    {
      // found, cumulate rssi samples
      peer_rssi_add_sample(&peer->second, rssi);
  
      // update nearest minutes
      int peerTimeDiff = time_diff(currentTimeToSecs, time_to_secs(&peer->second.firstSeen), SECS_PER_DAY);
      peer->second.mins = peerTimeDiff / 60;
      
      return true;
    }
  }

  // create in tempPeers
  {
    TS_Peer peer;
    peer.org = org;
    peer.deviceType = deviceType;
    peer.firstSeen = *current;

    peer_rssi_add_sample(&peer, rssi);
    
    this->tempPeers[ id ] = peer;
    return true;
  }
}

TS_PeerIterator* _TS_Storage::peer_get_next(TS_PeerIterator* it)
{
  if(it == NULL)
  {
    // log_i("DEBUG: new iterator");
    
    // Initialize new iterator
    it = new TS_PeerIterator();

    // List all files of /p/[mmdd]
    // Ignore files of /p/[mmdd]/*
    File root = StorageFFat::openDir("/p");
    if(root)
    {
      // log_i("DEBUG: root: %s", root.name());
      
      File file = root.openNextFile();
      while(file)
      {
        // log_i("DEBUG: filename %s", file.name());
        
        if(strlen(file.name()) == 7)
        {
          // a peer file
          it->dayFileNames.push_back(std::string(file.name()));
        }
        
        file.close();
        file = root.openNextFile();
      }
      
      root.close();
    }
  }

  // Reset file ptrs if they still exist
  it->validPeer = false;
  it->validIncident = false;
  if(it->fileId)
  {
    it->fileId.close();
  }
  
  if(it->fileIncident)
  {
    it->fileIncident.close();
  }

  // log_i("DEBUG: dayFileNames size: %d", it->dayFileNames.size());

  if(it->dayFileNames.size() > 0)
  {
    // Get next dayfile
    it->dayFileName = it->dayFileNames.front();
    it->dayFileNames.pop_front();
  
    it->fileId = StorageFFat::openRead(it->dayFileName.c_str());
    if(!it->fileId)
    {
      log_e("Failed to open file %s for r", it->dayFileName.c_str());
      delete it;
      return NULL;
    }
  
    // get first incident
    this->peer_get_next_peer(it);
    return it;
  }

  return it;
}

TS_PeerIterator* _TS_Storage::peer_get_next_peer(TS_PeerIterator* it)
{
  if(it == NULL) return NULL;
  it->validPeer = false;
  it->validIncident = false;
  if(!it->fileId)
  {
    log_e("Peer file not open");
    return it;
  }
  
  if(it->fileIncident)
  {
    it->fileIncident.close();
  }

  // log_i("DEBUG: peer_get_next_peer");
  
  // Get the next entry in file
  // File is /p/[mmdd]
  // - tempid,id,org,deviceType\n
  if(it->fileId.available())
  {
    log_i("DEBUG: fileid available: %d", it->fileId.available());
    
    it->peerId = std::string( it->fileId.readStringUntil(',').c_str() );
    it->peer.id = atoi( it->fileId.readStringUntil(',').c_str() );
    it->peer.org = std::string( it->fileId.readStringUntil(',').c_str() );
    it->peer.deviceType = std::string( it->fileId.readStringUntil('\n').c_str() );

    char filename[14];
    sprintf(filename, "%s/%d", it->dayFileName.c_str(), it->peer.id);

    log_i("DEBUG: filename: %s", filename);
    
    it->fileIncident = StorageFFat::openRead(filename);
    if(!it->fileIncident)
    {
      log_e("Failed to open file %s for r", filename);
      return it;
    }

    it->validPeer = true;

    // get next incident
    this->peer_get_next_incident(it);
  }

  return it;
}

TS_PeerIterator* _TS_Storage::peer_get_next_incident(TS_PeerIterator* it)
{
  if(it == NULL) return NULL;
  it->validIncident = false;
  if(!it->fileIncident)
  {
    log_e("Peer Incident file not open");
    return it;
  }

  // Get the next entry in file
  if(it->fileIncident.available())
  {
    PeerIncidentFileFrame frame;
    it->fileIncident.read((byte *)&frame, sizeof(frame));

    // populate peer data
    it->peer.firstSeen = frame.firstSeen;
    it->peer.mins = frame.mins;
    it->peer.rssi_min = frame.rssi_min;
    it->peer.rssi_max = frame.rssi_max;
    it->peer.rssi_sum = frame.rssi_sum;
    it->peer.rssi_samples = frame.rssi_samples;
    it->peer.rssi_dsquared = frame.rssi_dsquared;

    it->validIncident = true;
  }

  return it;
}

int _TS_Storage::peer_prune(uint8_t days, TS_DateTime *current)
{
  // prune files which are older than [days]
  char filename[40];
  int removed = 0;

  // List all files of /p/[mmdd]
  File root = StorageFFat::openDir("/p");
  if(root)
  {
    log_i("DEBUG: root: %s", root.name());
    
    File file = root.openNextFile();
    while(file)
    {
      log_i("DEBUG: file: %s", file.name());
      
      // filenames expected to be <= 32 char     
      strncpy(filename, file.name(), sizeof(filename));
      file.close();

      if(filename_older_than(filename, days, current))
      {
        StorageFFat::deleteFile(filename);
        ++removed;
      }
      
      file = root.openNextFile();
    }
    
    root.close();
  }
  
  return removed;
}

uint16_t _TS_Storage::peer_cleanup(TS_DateTime *current)
{
  int16_t tDiff = time_diff(current->minute, lastCleanupMins, SECS_PER_MIN);
  uint16_t entriesRemoved = 0;

  int now = time_to_secs(current);
  bool lowmem = LOWMEM_COND;
  int tempPeersSize = this->tempPeers.size();
  int peerCacheSize = this->peerCache.size();

  // Check conditions for cleanup
  if(!lowmem && !(tDiff > CLEANUP_MINS) && !(tempPeersSize > TEMPPEERS_MAX) && !(peerCacheSize > PEERCACHE_MAX))
  {
    return 0;
  }

  log_i("cache_cleanup start, free memory: %d (%d)", FREEMEM, FREEMEM_PCT);

  // To save memory, we don't have an LRU list but simply iterate and strive to remove at most two oldest entries of tempPeers if TEMPPEERS_MAX is exceeded
  // remove tempPeers which are old and probably would've timed out anyway
  {
    auto oldest = LargestN<int, std::string>(2);
    for(auto peer = this->tempPeers.begin(); peer != this->tempPeers.end();)
    {
      int peerTimeDiff = time_diff(time_to_secs(&peer->second.firstSeen), now, SECS_PER_DAY);
      int peerIdleDiff = peerTimeDiff + (peer->second.mins * 60);
      if(peerTimeDiff > TEMPPEERS_MAXAGE || peerIdleDiff > PEER_MAXIDLE)
      {
        this->tempPeers.erase(peer++);
        continue;
      }
      
      if(this->tempPeers.size() > TEMPPEERS_MAX)
      {
        // track oldest 2 entries to remove
        oldest.consider(peerTimeDiff, peer->first);
      }
  
      ++peer;
    }
  
    if(this->tempPeers.size() > TEMPPEERS_MAX)
    {
      auto keysPtr = oldest.getKeys();
      for(auto key = keysPtr->begin(); key != keysPtr->end(); ++key)
      {
        this->tempPeers.erase(*key);
      }
    }

    int tempPeersRemoved = tempPeersSize - this->tempPeers.size();
    entriesRemoved += tempPeersRemoved;
    log_i("Entries removed from tempPeers: %d", tempPeersRemoved);
  }

  // flush peerCache which are old and should be committed to flash
  {
    auto oldest = LargestN<int, std::string>(2);
    for(auto peer = this->peerCache.begin(); peer != this->peerCache.end();)
    {
      int peerTimeDiff = time_diff(time_to_secs(&peer->second.firstSeen), now, SECS_PER_DAY);
      int peerIdleDiff = peerTimeDiff + (peer->second.mins * 60);
      if(peerTimeDiff > PEERCACHE_MAXAGE || peerIdleDiff > PEER_MAXIDLE)
      {
        // commit and remove old entries
        auto tPeer = peer++;
        this->peer_cache_commit(tPeer->first, &tPeer->second, current);
        continue;
      }

      if(this->peerCache.size() > PEERCACHE_MAX)
      {
        // track oldest 2 entries to commit
        oldest.consider(peerTimeDiff, peer->first);
      }

      ++peer;
    }

    if(this->peerCache.size() > PEERCACHE_MAX)
    {
      auto keysPtr = oldest.getKeys();
      for(auto key = keysPtr->begin(); key != keysPtr->end(); ++key)
      {
        this->peer_cache_commit(*key, &this->peerCache[*key], current);
      }
    }

    int peerCacheRemoved = peerCacheSize - this->peerCache.size();
    entriesRemoved += peerCacheRemoved;
    log_i("Entries removed from peerCache: %d", peerCacheRemoved);
  }

  lastCleanupMins = current->minute;

  log_i("cache_cleanup end, free memory: %d (%d)", FREEMEM, FREEMEM_PCT);
  return entriesRemoved;
}



void _TS_Storage::peer_cache_commit(const std::string &key, TS_Peer *peer, TS_DateTime *current)
{
  // get id
  uint16_t id = this->peer_id_get_or_add(key, peer);
  peer->id = id;
  
  // append to entry
  this->peer_incident_add(peer);

  // erase from peercache
  this->peerCache.erase(key);
}



int _TS_Storage::peer_cache_commit_all(TS_DateTime *current)
{
  int count = 0;
  for(auto peer = this->peerCache.begin(); peer != this->peerCache.end();)
  {
    // save iterator as commit will erase entry
    auto tmpPeer = peer++;
    this->peer_cache_commit(tmpPeer->first, &tmpPeer->second, current);
    ++count;
  }

  return count;
}

//
// Peer file functions
//

uint16_t _TS_Storage::peer_id_get_or_add(const std::string &tempId, TS_Peer *peer)
{
  // TODO: should we wipe file if corrupted?
  
  // File is /p/[mmdd]
  // - tempid,id,org,deviceType\n
  
  char filename[10];
  sprintf(filename, "/p/%02d%02d", peer->firstSeen.month, peer->firstSeen.day);
  uint16_t id = 0;
  String dummy;

  // Open file for read
  File f = StorageFFat::openAppend(filename);
  if(!f)
  {
    log_e("Failed to open file %s for a+", filename);
    return 0;
  }

  // file exist, look for tempid
  while(f.available())
  {
    String fTid = f.readStringUntil(',');
    String fId = f.readStringUntil(',');
    dummy = f.readStringUntil(',');
    dummy = f.readStringUntil('\n');

    // take note of last id
    id = atoi(fId.c_str());

    if(tempId.compare(fTid.c_str()) == 0)
    {
      // found
      f.close();
      return id;
    }
  }

  // use last id +1
  ++id;

  // append new entry
  f.printf("%s,%d,%s,%s\n", tempId.c_str(), id, peer->org.c_str(), peer->deviceType.c_str());
  f.close();
  return id;
}

void _TS_Storage::peer_incident_add(TS_Peer *peer)
{
  // File is /p/[mmdd]/[id]
  // - tempid,id,org,deviceType\n
  
  char filename[14];
  sprintf(filename, "/p/%02d%02d/%d", peer->firstSeen.month, peer->firstSeen.day, peer->id);

  // Open file for append
  File f = StorageFFat::openAppend(filename);
  if(!f)
  {
    log_e("Failed to open file %s for a+", filename);
    return;
  }

  PeerIncidentFileFrame frame = {
    .firstSeen = peer->firstSeen,
    .mins = peer->mins,
    .rssi_min = peer->rssi_min,
    .rssi_max = peer->rssi_max,
    .rssi_sum = peer->rssi_sum,
    .rssi_samples = peer->rssi_samples,
    .rssi_dsquared = peer->rssi_dsquared,
  };

  f.write((byte *)&frame, sizeof(frame));
  f.close();
}

void _TS_Storage::peer_rssi_add_sample(TS_Peer *peer, int8_t rssi)
{ 
  if(rssi < peer->rssi_min) peer->rssi_min = rssi;
  if(rssi > peer->rssi_max) peer->rssi_max = rssi;
  peer->rssi_sum += rssi;
  ++peer->rssi_samples;
  int16_t mean = peer->rssi_sum / peer->rssi_samples;
  int16_t ds = (rssi - mean) * (rssi - mean);
  peer->rssi_dsquared += ds;
}

bool _TS_Storage::filename_older_than(char * filename, int8_t days, TS_DateTime *current)
{
  // Expected filename: /p/[mmdd]*
  char tee[3] = {0};
  int month, day;
  
  if(strlen(filename) < 7) return false;
  
  tee[0] = filename[3];
  tee[1] = filename[4];

  if(!isdigit(tee[0]) || !isdigit(tee[1])) return false;
  month = atoi(tee);
  if(month < 1 || month > 12) return false;

  tee[0] = filename[5];
  tee[1] = filename[6];

  if(!isdigit(tee[0]) || !isdigit(tee[1])) return false;
  day = atoi(tee);
  if(day < 1 || day > 31) return false;

  int diff = mmdd_diff(current->month, current->day, month, day);
  //log_i("DEBUG: diff: %d", diff);
  
  return (diff > days);
}

void _TS_Storage::set_default_settings()
{ 
  auto settings = this->settings_get();

  bool settingsUpdated = false;

  if (strcmp(settings->userId, "") == 0)
  {
    memset(settings->userId, '\0', sizeof(settings->userId));
    strcpy(settings->userId, DEFAULT_UID);
    settingsUpdated = true;
    log_i("Setting default UID");
  }

  if (strcmp(settings->wifiSsid, "") == 0)
  {
    memset(settings->wifiSsid, '\0', sizeof(settings->wifiSsid));
    strcpy(settings->wifiSsid, WIFI_SSID);
    settingsUpdated = true;
    log_i("Setting default WIFI_SSID");
  }

  if (strcmp(settings->wifiPass, "") == 0)
  {
    memset(settings->wifiPass, '\0', sizeof(settings->wifiPass));
    strcpy(settings->wifiPass, WIFI_PASS);
    settingsUpdated = true;
    log_i("Setting default WIFI_PASS");
  }

  log_i("Stored settings:");
  log_i("Version: %d", settings->settingsVersion);
  log_i("UID: %.32s", settings->userId);
  log_i("WIFI_SSID: %.32s", settings->wifiSsid);
  log_i("WIFI_PASS: %.32s", settings->wifiPass);

  if (settingsUpdated)
  {
    this->settings_save();
  }
}

//
// Testing functions
//

bool _TS_StorageTests::test_ffat()
{
  return StorageFFat::testFileIO("/test.txt");
}



