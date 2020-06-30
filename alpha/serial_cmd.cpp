#include <stdio.h>
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "argtable3/argtable3.h"
#include "hal.h"
#include "serial_cmd.h"


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
} clock_args;

static int do_clock_cmd(int argc, char **argv)
{
  TS_DateTime dt;

  int nerrors = arg_parse(argc, argv, (void **) &clock_args);
  if (nerrors != 0)
  {
    arg_print_errors(stderr, clock_args.end, argv[0]);
    return ESP_ERR_INVALID_ARG;
  }

  if (clock_args.get->count == 1)
  {
    TS_HAL.rtc_get(dt);
    printf("Current datetime: %04d-%02d-%02dT%02d:%02d:%02d\n\n", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
  } 
  else if (clock_args.set->count == 1)
  {
    /* tm_year gives years since 1900 */
    dt.year = (clock_args.set->tmval->tm_year) + 1900;
    /* tm_mon range = [0, 11], convert to [1,12] */
    dt.month = (clock_args.set->tmval->tm_mon) + 1;
    dt.day = clock_args.set->tmval->tm_mday;
    dt.hour = clock_args.set->tmval->tm_hour;
    dt.minute = clock_args.set->tmval->tm_min;
    dt.second = clock_args.set->tmval->tm_sec;
    TS_HAL.rtc_set(dt);
    printf("Success!\n\n");
  } 
  else
  {
    printf("Type help for help\n\n");
    return ESP_ERR_INVALID_ARG;
  }

  return ESP_OK;
}

static void register_clock_cmd()
{
  clock_args.get = arg_lit0("g", "get", "get datetime");
  clock_args.set = arg_date0("s", "set", "%Y-%m-%dT%H:%M:%S", NULL, "set datetime");
  clock_args.end = arg_end(20);

  const esp_console_cmd_t cmd =
  {
    .command = "clock",
    .help = "Get or set datetime",
    .hint = NULL,
    .func = &do_clock_cmd,
    .argtable = &clock_args
  };
  ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

//
// Register command callbacks
// - this has to be at the end after all static functions
//
void _TS_SerialCmd::register_commands()
{
  register_version();
  register_clock_cmd();
  esp_console_register_help_command();
}

