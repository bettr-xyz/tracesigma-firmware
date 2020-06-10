#ifndef __TS_UI__
#define __TS_UI__

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
    void task(void*);
    bool read_button(bool, button&);

};

extern _TS_UI TS_UI;


#endif
