#include <stdio.h>
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "hal.h"
#include "serial_cmd.h"
#include "storage.h"


// Increase as UI thread uses more things
#define THREAD_STACK_SIZE 5000

_TS_SerialCmd TS_SerialCmd;
_TS_SerialCmd::_TS_SerialCmd()
{
  uart_initialized = false;
  
}

void _TS_SerialCmd::init()
{
  init_console();
  register_commands();
}

void _TS_SerialCmd::begin()
{
  // Serial command thread does not need much time
  xTaskCreatePinnedToCore(
    _TS_SerialCmd::staticTask, // thread fn
    "SerialCmdTask",           // identifier
    THREAD_STACK_SIZE,  // stack size
    NULL,               // parameter
    2,                  // increased priority (main loop is running at priority 1, idle is 0, ui is at 2)
    NULL,               // handle
    1);                 // core
  
}

void _TS_SerialCmd::staticTask(void* parameter)
{
  uint8_t data;
  while(true)
  {
    if(uart_read_bytes(UART_NUM_0, &data, 1, 100) > 0)
    {
      TS_SerialCmd.serial_cmd_loop();
    }

    // TODO: sleep longer while serial is not in use e.g. 1000 ms
    TS_HAL.sleep(TS_SleepMode::Task, 100);
  }
}

void _TS_SerialCmd::init_console()
{
  /* Disable buffering on stdin */
  setvbuf(stdin, NULL, _IONBF, 0);

  /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
  esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  /* Move the caret to the beginning of the next line on '\n' */
  esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

  /* Tell VFS to use UART driver */
  esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

  /* Initialize the console */
  esp_console_config_t console_config;
  console_config.max_cmdline_args = 8;
  console_config.max_cmdline_length = 256;
  
  ESP_ERROR_CHECK( esp_console_init(&console_config) );

  /* Configure linenoise line completion library */
  /* Enable multiline editing. If not set, long commands will scroll within
   * single line.
   */
  linenoiseSetMultiLine(1);

  /* Tell linenoise where to get command completions and hints */
  linenoiseSetCompletionCallback(&esp_console_get_completion);
  linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

  /* Set command history size */
  // disable history, save memory
  linenoiseHistorySetMaxLen(0);

  // no history
  // #if CONFIG_STORE_HISTORY
  /* Load command history from filesystem */
  // linenoiseHistoryLoad(HISTORY_PATH);
  // #endif
  
}
    
void _TS_SerialCmd::serial_cmd_loop()
{
  /* Prompt to be printed before each line.
   * This can be customized, made dynamic, etc.
   */
  const char* prompt = "esp32> ";
  linenoiseSetDumbMode(1);

  printf("\n"
         "This is an example of ESP-IDF console component.\n"
         "Type 'help' to get the list of commands.\n"
         "Use UP/DOWN arrows to navigate through command history.\n"
         "Press TAB when typing command name to auto-complete.\n");

  /* Main loop */
  while(true) {
     /* Get a line using linenoise.
       * The line is returned when ENTER is pressed.
       */
      char* line = linenoise(prompt);
      if (line == NULL) { /* Ignore empty lines */
          continue;
      }

      /* Try to run the command */
      int ret;
      esp_err_t err = esp_console_run(line, &ret);
      if (err == ESP_ERR_NOT_FOUND) {
          printf("Unrecognized command\n");
      } else if (err == ESP_ERR_INVALID_ARG) {
          // command was empty
      } else if (err == ESP_OK && ret != ESP_OK) {
          printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
      } else if (err != ESP_OK) {
          printf("Internal error: %s\n", esp_err_to_name(err));
      }
      /* linenoise allocates line buffer on the heap, so need to free it */
      linenoiseFree(line);
  }
}

static void print_cmd_help(char* cmd_name, void** argtable)
{
  printf("%s ",cmd_name);
  arg_print_syntax(stdout, argtable, "\n");
  arg_print_glossary_gnu(stdout, argtable);
}

static void save_to_ram_or_eeprom(struct arg_lit *ram_flag)
{
  printf("Saved to: ");
  if (ram_flag->count == 1)
  {
    printf("RAM\n\n");
  }
  else
  {
    TS_Storage.settings_save();
    printf("EEPROM\n\n");
  }
}

//
// Copy input string to TS_Settings if length check passes
// - in_str: arg.{cmd}->sval[n]
// - store_loc: TS_Settings->{str_setting}
//
static int check_copy_str_setting(const char* in_str, char* store_loc)
{
  if (strlen(in_str) <= STR_ARG_MAXLEN)
  {
    strcpy(store_loc, in_str);
  }
  else
  {
    printf("String argument exceeds max length of %d\n\n", STR_ARG_MAXLEN);
  }
}

static int get_version(int argc, char **argv)
{
  esp_chip_info_t info;
  esp_chip_info(&info);
  printf("IDF Version:%s\r\n", esp_get_idf_version());
  printf("Chip info:\r\n");
  printf("\tmodel:%s\r\n", info.model == CHIP_ESP32 ? "ESP32" : "Unknow");
  printf("\tcores:%d\r\n", info.cores);
  printf("\tfeature:%s%s%s%s\r\n",
         info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "",
         info.features & CHIP_FEATURE_BLE ? "/BLE" : "",
         info.features & CHIP_FEATURE_BT ? "/BT" : "",
         info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:");
  printf("\trevision number:%d\r\n", info.revision);
  return 0;
}

static void register_version()
{
  const esp_console_cmd_t cmd = {
      .command = "version",
      .help = "Get version of chip and SDK",
      .hint = NULL,
      .func = &get_version,
  };
  ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static struct
{
  struct arg_lit *get;
  struct arg_date *set;
  struct arg_end *end;
} clockArgs;

static int do_clock_cmd(int argc, char **argv)
{
  TS_DateTime dt;

  int nerrors = arg_parse(argc, argv, (void **) &clockArgs);
  if (nerrors != 0)
  {
    arg_print_errors(stderr, clockArgs.end, argv[0]);
    return ESP_ERR_INVALID_ARG;
  }

  if (clockArgs.get->count == 1)
  {
    TS_HAL.rtc_get(dt);
    printf("Current datetime: %04d-%02d-%02dT%02d:%02d:%02d\n\n", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
  } 
  else if (clockArgs.set->count == 1)
  {
    /* tm_year gives years since 1900 */
    dt.year = (clockArgs.set->tmval->tm_year) + 1900;
    /* tm_mon range = [0, 11], convert to [1,12] */
    dt.month = (clockArgs.set->tmval->tm_mon) + 1;
    dt.day = clockArgs.set->tmval->tm_mday;
    dt.hour = clockArgs.set->tmval->tm_hour;
    dt.minute = clockArgs.set->tmval->tm_min;
    dt.second = clockArgs.set->tmval->tm_sec;
    TS_HAL.rtc_set(dt);
    printf("Success!\n\n");
  } 
  else
  {
    print_cmd_help(argv[0], (void**) &clockArgs);
  }

  return ESP_OK;
}

static void register_clock_cmd()
{
  clockArgs.get = arg_lit0("g", "get", "get datetime");
  clockArgs.set = arg_date0("s", "set", "%Y-%m-%dT%H:%M:%S", NULL, "set datetime (e.g. clock -s 2020-12-31T11:22:33");
  clockArgs.end = arg_end(20);

  const esp_console_cmd_t cmd =
  {
    .command = "clock",
    .help = "Get or set datetime",
    .hint = NULL,
    .func = &do_clock_cmd,
    .argtable = &clockArgs
  };
  ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static struct
{
  struct arg_lit *get;
  struct arg_str *ssid;
  struct arg_str *password;
  struct arg_lit *ram_flag;
  struct arg_end *end;
} wifiArgs;

static int do_wifi_cmd(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void**) &wifiArgs);

  if (nerrors != 0)
  {
    arg_print_errors(stderr, wifiArgs.end, argv[0]);
    return ESP_ERR_INVALID_ARG;
  }

  /* get settings from eeprom */
  TS_Settings* settings = TS_Storage.settings_get();

  /* user issued get cmd */
  if (wifiArgs.get->count == 1)
  {
    printf("SSID: %s\n", settings->wifiSsid);
    printf("Password: %s\n\n", settings->wifiPass);
  }
  /* user issued set cmd, store string if length check passes */
  else if (wifiArgs.ssid->count == 1 || wifiArgs.password->count == 1)
  {
    if (wifiArgs.ssid->count == 1)
    {
      check_copy_str_setting(wifiArgs.ssid->sval[0], settings->wifiSsid);
    }
    if (wifiArgs.password->count == 1)
    {
      check_copy_str_setting(wifiArgs.password->sval[0], settings->wifiPass);
    }
    save_to_ram_or_eeprom(wifiArgs.ram_flag);
  }
  /* user didn't provide args, print help */
  else
  {
    print_cmd_help(argv[0], (void**) &wifiArgs);
  }

  return ESP_OK;
}

static void register_wifi_cmd(void)
{
  wifiArgs.get = arg_lit0("g", "get", "get WIFI settings");
  wifiArgs.ssid = arg_str0("s", "ssid", NULL, "set SSID, wrap in quotes \"\" if contains space");
  wifiArgs.password = arg_str0("p", "pass", NULL, "set password");
  wifiArgs.ram_flag = arg_lit0("r", "ram", "[debug] save to RAM not EEPROM, will not persist after power cycle (e.g. wifi -s abc -r)");
  wifiArgs.end = arg_end(20);

  const esp_console_cmd_t sta_cmd =
  {
    .command = "wifi",
    .help = "Get or set WIFI settings",
    .hint = NULL,
    .func = &do_wifi_cmd,
    .argtable = &wifiArgs
  };

  ESP_ERROR_CHECK( esp_console_cmd_register(&sta_cmd) );
}

static struct
{
  struct arg_lit *get;
  struct arg_int *upload_flag;
  struct arg_lit *ram_flag;
  struct arg_end *end;
} flagArgs;

static void print_flags(TS_Settings* settings)
{
  printf("upload: %d\n\n", 1 ? settings->upload_flag : 0);
}

static int do_flag_cmd(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void **) &flagArgs);
  if (nerrors != 0)
  {
    arg_print_errors(stderr, flagArgs.end, argv[0]);
    return ESP_ERR_INVALID_ARG;
  }

  TS_Settings* settings = TS_Storage.settings_get();

  if (flagArgs.get->count == 1)
  {
    print_flags(settings);
  }
  else if (flagArgs.upload_flag->count == 1)
  {
    switch (flagArgs.upload_flag->ival[0])
    {
      case 0:
        settings->upload_flag = false;
        break;
      case 1:
        settings->upload_flag = true;
        break;
      default:
        print_cmd_help(argv[0], (void**) &flagArgs);
        return ESP_ERR_INVALID_ARG;
    }
    save_to_ram_or_eeprom(flagArgs.ram_flag);
  }
  else
  {
    print_cmd_help(argv[0], (void**) &flagArgs);
  }

  return ESP_OK;
}

static void register_flag_cmd()
{
  flagArgs.get = arg_lit0("g", "get", "get flag settings");
  flagArgs.upload_flag = arg_int0("u", "upload", "<int>", "1: upload temp IDs when WIFI connected, 0: disabled");
  flagArgs.ram_flag = arg_lit0("r", "ram", "[debug] save to RAM not EEPROM, will not persist after power cycle (e.g. flag -u 1 -r)");
  flagArgs.end = arg_end(20);

  const esp_console_cmd_t cmd =
  {
    .command = "flag",
    .help = "Get or set flags",
    .hint = NULL,
    .func = &do_flag_cmd,
    .argtable = &flagArgs
  };
  ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static struct
{
  struct arg_lit *get;
  struct arg_str *set;
  struct arg_lit *ram_flag;
  struct arg_end *end;
} useridArgs;

static int do_userid_cmd(int argc, char **argv)
{
  int nerrors = arg_parse(argc, argv, (void**) &useridArgs);

  if (nerrors != 0)
  {
    arg_print_errors(stderr, useridArgs.end, argv[0]);
    return ESP_ERR_INVALID_ARG;
  }

  /* get settings from eeprom */
  TS_Settings* settings = TS_Storage.settings_get();

  /* user issued get cmd */
  if (useridArgs.get->count == 1)
  {
    printf("userID: %s\n", settings->userId);
  }
  /* user issued set cmd, store string if length check passes */
  else if (useridArgs.set->count == 1)
  {
    check_copy_str_setting(useridArgs.set->sval[0], settings->userId);
    save_to_ram_or_eeprom(useridArgs.ram_flag);
  }
  /* user didn't provide args, print help */
  else
  {
    print_cmd_help(argv[0], (void**) &useridArgs);
  }

  return ESP_OK;
}

static void register_userid_cmd(void)
{
  useridArgs.get = arg_lit0("g", "get", "get userID");
  useridArgs.set = arg_str0("s", "set", NULL, "set userID");
  useridArgs.ram_flag = arg_lit0("r", "ram", "[debug] save to RAM not EEPROM, will not persist after power cycle (e.g. userid -s abc -r)");
  useridArgs.end = arg_end(20);

  const esp_console_cmd_t sta_cmd =
  {
    .command = "userid",
    .help =  "Get or set userID",
    .hint = NULL,
    .func = &do_userid_cmd,
    .argtable = &useridArgs
  };

  ESP_ERROR_CHECK( esp_console_cmd_register(&sta_cmd) );
}

//
// Register command callbacks
// - this has to be at the end after all static functions
//
void _TS_SerialCmd::register_commands()
{
  register_clock_cmd();
  register_flag_cmd();
  register_userid_cmd();
  register_version();
  register_wifi_cmd();
  esp_console_register_help_command();
}

