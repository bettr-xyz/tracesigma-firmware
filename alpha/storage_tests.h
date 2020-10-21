//
// Storage Tests
//

#ifndef __TS_STORAGE_TESTS__
#define __TS_STORAGE_TESTS__

#if defined(TESTDRIVER) && defined(TESTDRIVER_STORAGE)

static class _TS_StorageTests : public _TS_Tests
{  
public:
  std::string test_id;
  std::string test_org;
  std::string test_device;
  TS_DateTime test_time;
  int8_t test_rssi;

  bool test_peer_log_pass;
  bool test_iterate_logs_one_pass;

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
    test_iterate_logs_one_pass = false;
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

  bool test_peer_log_2()
  { 
    test_peer_log_pass = false;

    for(int i = 1; i <= 5; ++i)
    {
      test_time.minute = 10 + i;
      if( !TS_Storage.peer_log_incident( test_id, test_org, test_device, test_rssi, &test_time ) )
      {
        log_e("peer_log_incident_2 mins+%d expected to return true", i);
        return false;
      }
    }

    test_peer_log_pass = true;
    return true;
  }
  
  // test iterate
  bool test_iterate_logs_none()
  {
    if(!test_peer_log_pass)
    {
      log_e("Test depends on test_peer_log passing");
      return false;
    }

    auto it = TS_Storage.peer_get_next(NULL);
    if(it == NULL)
    {
      log_e("Null PeerIterator");
      return false;
    }

    int ret = it->log();
    delete it;
    if(ret != 0)
    {
      log_e("Expected no logs/storage found!");
      return false;
    }

    return true;
  }

  // test iterate : expect one log file
  bool test_iterate_logs_one()
  {
    test_iterate_logs_one_pass = false;
    if(!test_peer_log_pass)
    {
      log_e("Test depends on test_peer_log passing");
      return false;
    }

    // TODO: we need to delete `it` before leaving
    auto it = TS_Storage.peer_get_next(NULL);
    if(it == NULL)
    {
      log_e("Null PeerIterator");
      return false;
    }

    int ret = it->log();
    if(ret != 0)
    {
      it = TS_Storage.peer_get_next(it);
      int ret2 = it->log();
      
      if(ret2 == 0)
      {
        test_iterate_logs_one_pass = true;
        delete it;
        return true;
      }

      log_e("Second log file found");
    }

    log_e("Expected exactly one log file!");
    delete it;
    return false;
  }

  // test cleanup
  bool test_cleanup_before_elapsed()
  {
    TS_Storage.lastCleanupMins = 10;  // 0 mins elapsed
    if (TS_Storage.peer_cleanup(&test_time) != 0)
    {
      log_e("Cleanup should not have happened");
      return false;
    }

    TS_Storage.lastCleanupMins = 10;  // 3+ mins elapsed
    test_time.minute = 13;
    if (TS_Storage.peer_cleanup(&test_time) != 0)
    {
      log_e("Cleanup should not have removed anything as PEER_MAXIDLE has not elapsed");
      return false;
    }

    return true;
  }

  bool test_cleanup_after_elapsed()
  {
    if(!test_peer_log_pass)
    {
      log_e("Test depends on test_peer_log passing");
      return false;
    }

    TS_Storage.lastCleanupMins = 10;  // 4+ mins elapsed
    test_time.minute = 14;
    if (TS_Storage.peer_cleanup(&test_time) != 1)
    {
      log_e("Cleanup should have removed the one and only entry");
      return false;
    }
    
    return true;
  }

  bool test_cleanup_after_elapsed_2()
  {
    if(!test_peer_log_pass)
    {
      log_e("Test depends on test_peer_log passing");
      return false;
    }

    TS_Storage.lastCleanupMins = 16;  // 6 mins elapsed
    test_time.minute = 15;
    if (TS_Storage.peer_cache_commit_all(&test_time) != 1)
    {
      log_e("Cache commit should have committed exactly 1");
      return false;
    }
    
    return true;
  }

  bool test_prune_noop()
  {
    // calls prune but nothing should happen
    int pruned = TS_Storage.peer_prune(10, &test_time);
    if(pruned == 0)
    {
      return true;
    }

    log_e("Prune should have removed 0 files, %d files removed instead", pruned);
    return false;
  }

  bool test_prune_all()
  {    
    // prune all files
    int pruned = TS_Storage.peer_prune(-1, &test_time);
    
    TS_Storage.reset();
    
    if(pruned == 1)
    {
      return true;
    }

    log_e("Prune should have removed 1 file, %d files removed instead", pruned);
    return false;
  }

  // Ctor
  _TS_StorageTests()
  { 
    add(std::bind(&_TS_StorageTests::test_peer_log, this), "test_peer_log");
    add(std::bind(&_TS_StorageTests::test_iterate_logs_none, this), "test_iterate_logs_none");
    add(std::bind(&_TS_StorageTests::test_cleanup_before_elapsed, this), "test_cleanup_before_elapsed");
    add(std::bind(&_TS_StorageTests::test_cleanup_after_elapsed, this), "test_cleanup_after_elapsed");

    // test_peer_log again w/ different time to force write to file
    add(std::bind(&_TS_StorageTests::test_peer_log, this), "test_peer_log");
    add(std::bind(&_TS_StorageTests::test_peer_log_2, this), "test_peer_log_2");
    add(std::bind(&_TS_StorageTests::test_cleanup_after_elapsed_2, this), "test_cleanup_after_elapsed_2");
    
    // Iterate again when files actually exist
    add(std::bind(&_TS_StorageTests::test_iterate_logs_one, this), "test_iterate_logs_one");
    
    // test pruning
    add(std::bind(&_TS_StorageTests::test_prune_noop, this), "test_prune_noop");
    add(std::bind(&_TS_StorageTests::test_prune_all, this), "test_prune_all");
  }
} TS_StorageTests;

#endif
#endif
