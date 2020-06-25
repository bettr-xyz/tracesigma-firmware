#ifndef __TS_SERIAL__
#define __TS_SERIAL__

#define CONFIG_ESP_CONSOLE_UART_NUM 0
#define STR_ARG_MAXLEN 31


class _TS_SerialCmd
{
  public:
    _TS_SerialCmd();

    void init();
    void begin();
    
    static void staticTask(void*);
    
    void init_console();
    
    void serial_cmd_loop();

    void register_commands();

  private:
    bool uart_initialized;
};

extern _TS_SerialCmd TS_SerialCmd;

#endif
