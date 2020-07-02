//
// Common functions
//

#ifndef __TS_CLEANBOX__
#define __TS_CLEANBOX__

#include "hal.h"
#include <list>

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

#define FREEMEM ESP.getFreeHeap()

// Max mem is 327680B
#define FREEMEM_PCT FREEMEM / 3276

#define LOWMEM 20480

#define LOWMEM_COND (FREEMEM < LOWMEM)

inline int time_to_secs(TS_DateTime *t)
{
  return t->second + t->minute*60 + t->hour*3600;
}

inline int time_diff(int t1, int t2, int wrap)
{
  int t = t1 - t2;
  return t < 0
    ? t + wrap  
    : t;
}

// Helper class to track smallest N values
// - `front` always contains smallest
// - other elements in the may not be in order, but will be smaller than `front`
template <typename T, typename U> class SmallestN
{
  public:
    SmallestN(uint8_t sizeLimit, bool compareLarger)
    : larger(compareLarger), limit(sizeLimit)
    {
    };
    
    SmallestN(uint8_t sizeLimit)
    : SmallestN(sizeLimit, false)
    {
    };

    void consider(T value, U key)
    {
      if(smallestValues.size() == 0 ||
        (value <= smallestValues.front() && !larger) ||
        (value >= smallestValues.front() && larger))
      {
        smallestValues.push_front(value);
        smallestKeys.push_front(key);
      }
      else
      {
        smallestValues.push_back(value);
        smallestKeys.push_back(key);
      }

      if(smallestValues.size() > limit)
      {
        smallestValues.pop_back();
        smallestKeys.pop_back();
      }
    };

    std::list<T> *getValues()
    {
      return &smallestValues;
    }

    std::list<U> *getKeys()
    {
      return &smallestKeys;
    }
    
  protected:
    bool larger;
    uint8_t limit;
    std::list<T> smallestValues;
    std::list<U> smallestKeys;
};

template <typename T, typename U> class LargestN : public SmallestN<T,U>
{
  public:
    LargestN(uint8_t sizeLimit)
    : SmallestN<T,U>(sizeLimit, true)
    {
    };
};

#endif
