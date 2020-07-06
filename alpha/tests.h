// TEST: Define TESTDRIVER to enable test execution mode
#define TESTDRIVER

#ifdef TESTDRIVER

#ifndef __TS_TESTS__
#define __TS_TESTS__

#include <list>
#include <exception>
#include <functional>
#include <string>

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

    auto fnName = testFunctionNames.begin();
    for(auto fn = testFunctions.begin(); fn != testFunctions.end(); ++fn)
    {
      bool result;
      try
      {
        setup();
        long tsStart = millis();
        long tsEnd = tsStart;
        try
        {
          result = (*fn)();
          tsEnd = millis();
        }
        catch(std::exception e)
        {
          log_e("exception: %s", e.what());
        }
        
        log_i("Test %s: %s (%d ms)", fnName->c_str(), (result ? "PASS" : "FAIL"), (tsEnd-tsStart));
        teardown();
      }
      catch(std::exception ee)
      {
        log_e("Error in setup/teardown for test %s", fnName->c_str());
        log_e("exception: %s", ee.what());
      }

      ++fnName;
    }
  }  
};

#endif
#endif
