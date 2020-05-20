/* Console example — various system commands

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_ESP_CONSOLE_UART_NUM 0

void initialize_uart();
void initialize_console();
void register_commands();
void serial_cmd_loop();

#ifdef __cplusplus
}
#endif
