#include "storage.h"
#include "hal.h"
#include "cleanbox.h"
#include <EEPROM.h>

#define EEPROM_SIZE       1024
#define SETTINGS_VERSION  0x01

// Cleanup every 5 mins
#define CLEANUP_MINS      5

#define TEMPPEERS_MAX     100
#define PEERCACHE_MAX     30

#define TEMPPEERS_MAXAGE  420    // 7 min
#define PEERCACHE_MAXAGE  1080   // 18 min

#define SECS_PER_DAY      86400
#define SECS_PER_MIN      60



_TS_Storage TS_Storage;
_TS_Storage::_TS_Storage()
{
  lastCleanupMins = 0;
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
  
  // format on error, basepath, maxOpenFiles
  // - use the shortest path, f - spi flash
  if(!SPIFFS.begin(true, "/f", 1))
  {
    log_e("SPIFFS init error");

    // no point continuing
    return;
  }

  // dump all files
  log_i("Listing all files");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while(file){
    log_i("FILE: %s", file.name());
    file = root.openNextFile();
  }
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
  uint32_t total = SPIFFS.totalBytes();
  uint32_t used = SPIFFS.usedBytes();
  uint32_t pct = (used*100)/total;  // 0-100 range
  return (uint8_t)pct;
}

uint32_t _TS_Storage::freespace_get()
{
  uint32_t total = SPIFFS.totalBytes();
  return total - SPIFFS.usedBytes();
}

uint32_t _TS_Storage::usedspace_get()
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

bool _TS_Storage::peer_log_incident(std::string id, std::string org, std::string deviceType, int8_t rssi, TS_DateTime *current)
{
  // if exist in tempPeers, cumulate. If mins >= 5 , insert into peerCache/peermap, delete from tempPeers
  auto tempPeer = this->tempPeers.find(id);
  if (tempPeer != this->tempPeers.end())
  {
    // found
  }

  // TODO: evict old entries from tempPeers if low mem
  
  
  // TODO: if id does not exist in peerCache and peermap file, create in peermap and add to peerCache

  // TODO: if exist in tempPeers, cumulate
  

  
}

// TODO: other fns




uint16_t _TS_Storage::peer_cleanup(TS_DateTime *current)
{
  int16_t tDiff = time_diff(current->minute, lastCleanupMins, SECS_PER_MIN);
  uint16_t entriesRemoved = 0;

  int now = time_to_secs(current);
  bool lowmem = LOWMEM_COND;
  int tempPeersSize = this->tempPeers.size();

  // Check conditions for cleanup
  if(!lowmem && !(tDiff > CLEANUP_MINS) && !(tempPeersSize > TEMPPEERS_MAX) && !(this->peerCache.size() > PEERCACHE_MAX))
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
      if(peerTimeDiff > TEMPPEERS_MAXAGE)
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
  
    entriesRemoved += (tempPeersSize - this->tempPeers.size());
    log_i("Entries removed from tempPeers: %d", (tempPeersSize - this->tempPeers.size()));
  }

  // TODO: flush peerCache which are old and should be committed to flash



  log_i("cache_cleanup end, free memory: %d (%d)", FREEMEM, FREEMEM_PCT);
  return entriesRemoved;
}



bool _TS_Storage::peer_cache_commit(std::string key, TS_DateTime *current)
{

  // TODO:
  return true;
  
}





