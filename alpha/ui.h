#ifndef __TS_UI__
#define __TS_UI__

#include <stdint.h>

struct button {
  bool down;
  unsigned long nextClick;
};

class _TS_UI
{
  public:
    _TS_UI();
    void begin();

    static void staticTask(void*);


  private:
    int8_t batteryIconIndex;

    void task(void*);
    bool read_button(bool, button&);
    void draw_battery_icon();

};

extern _TS_UI TS_UI;


#endif
