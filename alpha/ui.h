#include "io.h"

#ifndef __TS_UI__
#define __TS_UI__

class _TS_UI
{
  public:
    _TS_UI();
    void begin();

    static void staticTask(void*);


  private:
    void task(void*);
    IOButton* buttonA;
    IOButton* buttonB;

};

extern _TS_UI TS_UI;


#endif
