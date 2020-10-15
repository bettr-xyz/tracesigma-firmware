// TEST: Define TESTDRIVER to enable test execution mode
//#define TESTDRIVER

// TEST: Define TESTDRIVER_STORAGE to enable STORAGE tests
#define TESTDRIVER_STORAGE

#ifdef TESTDRIVER

#ifndef __TS_TESTS__
#define __TS_TESTS__

#include "cleanbox.h"

#include <list>
#include <exception>
#include <functional>
#include <string>

#define TEST_REPEAT_INTERVAL    5000
#define TEST_REPEAT_ITERATIONS  3

//
// Rudimentary test framework
//

class _TS_Tests
{
protected:
  std::vector<std::function<bool()>> testFunctions;
  std::vector<std::string> testFunctionNames;

  virtual void add(std::function<bool()> test, std::string &name)
  {
    testFunctions.push_back(test);
    testFunctionNames.push_back(name);
  }

  virtual void add(std::function<bool()> test, const char *name)
  {
    testFunctions.push_back(test);
    testFunctionNames.push_back(std::string(name));
  }

public:
  _TS_Tests()
  {
  }

  virtual void init() = 0;
  virtual void setup() {}
  virtual void teardown() {}

  virtual void run_all()
  {
    init();

    int passed = 0;
    auto fnName = testFunctionNames.begin();
    for(auto fn = testFunctions.begin(); fn != testFunctions.end(); ++fn)
    {
      bool result;
      long tsStart;
      long tsEnd;

      setup();
      tsStart = millis();
      tsEnd = tsStart;
        
      result = (*fn)();
      tsEnd = millis();
        
      teardown();
     
      log_i("[%3dms,%6dB] Test %s: %s", (tsEnd-tsStart), FREEMEM, fnName->c_str(), (result ? "PASS" : "FAIL"));
      if (result) ++passed;
      ++fnName;
    }

    log_i("Tests passed: %d / %d", passed, testFunctions.size());
  }

  virtual void run_all_repeatedly()
  {
    for(uint8_t i = 0; i < TEST_REPEAT_ITERATIONS; ++i)
    {
      log_i("=======================", i+1);
      log_i("Test iteration %d ...", i+1);
      this->run_all();
      delay(TEST_REPEAT_INTERVAL);
    }

    // stop here
    while(1) delay(TEST_REPEAT_INTERVAL);
  }
};

#endif
#endif
