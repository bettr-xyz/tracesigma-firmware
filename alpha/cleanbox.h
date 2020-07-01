//
// Common functions
//

#ifndef __TS_CLEANBOX__
#define __TS_CLEANBOX__

#include "hal.h"

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

#endif
