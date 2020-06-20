# tracestick

https://sites.google.com/view/tracestick

## Getting Started (PlatformIO)

### Clone/Download Repository from Github

`
git clone https://github.com/bettr-xyz/tracestick-firmware.git
`

### Download and Install PlatformIO IDE

- Follow the instructions to install [PlatformIO as an extension of VSCode](https://docs.platformio.org/en/latest/integration/ide/vscode.html#ide-vscode).
- Run through [Quick Start](https://docs.platformio.org/en/latest/integration/ide/vscode.html#quick-start) if it is your first time using PlatformIO

### Build Project

<details>
	<summary>Click to view sample output </summary>

```
> Executing task in folder trackstick_firmware: C:\Users\User\.platformio\penv\Scripts\platformio.exe run <

Processing m5stick-c (platform: espressif32; board: m5stick-c; framework: arduino) 
----------------------------------------------------------------------------------------------------------------------------------------------------- 
PackageManager: Installing framework-arduinoespressif32
git version 2.19.0.windows.1
Cloning into 'C:\Users\User\.platformio\packages\_tmp_installing-stb0bp-package'...
remote: Enumerating objects: 2436, done.
remote: Counting objects: 100% (2436/2436), done.
remote: Compressing objects: 100% (1837/1837), done.
remote: Total 2436 (delta 506), reused 1610 (delta 384), pack-reused 0
Receiving objects: 100% (2436/2436), 41.66 MiB | 3.71 MiB/s, done.
Resolving deltas: 100% (506/506), done.
Checking out files: 100% (2042/2042), done.
Submodule 'libraries/AzureIoT' (https://github.com/VSChina/ESP32_AzureIoT_Arduino) registered for path 'libraries/AzureIoT'
Cloning into 'C:/Users/User/.platformio/packages/_tmp_installing-stb0bp-package/libraries/AzureIoT'...
remote: Enumerating objects: 201, done.
remote: Total 201 (delta 0), reused 0 (delta 0), pack-reused 201
Receiving objects: 100% (201/201), 314.48 KiB | 449.00 KiB/s, done.
Resolving deltas: 100% (34/34), done.
Submodule path 'libraries/AzureIoT': checked out '67dfa4f31ef88b0938dd87d955612100dea5562e'
framework-arduinoespressif32 @ b92c58d has been successfully installed!
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/m5stick-c.html
PLATFORM: Espressif 32 1.12.3 > M5Stick-C
HARDWARE: ESP32 240MHz, 320KB RAM, 4MB Flash
DEBUG: Current (esp-prog) External (esp-prog, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
PACKAGES:
 - framework-arduinoespressif32 b92c58d 
 - tool-esptoolpy 1.20600.0 (2.6.0) 
 - toolchain-xtensa32 2.50200.80 (5.2.0)
Converting alpha.ino
LDF: Library Dependency Finder -> http://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Looking for ArduinoJson library in registry
Found: https://platformio.org/lib/show/64/ArduinoJson
LibraryManager: Installing id=64
Downloading  [####################################]  100%
Unpacking  [################################loning into 'C:\Users\User\tracestick_alpha\.pio\libdeps\m5stick-c\_tmp_installing-hahgqs-package'...
Unpacking  [####################################]  100%          
ArduinoJson @ 6.15.2 has been successfully installed!
LibraryManager: Installing M5StickC
git version 2.19.0.windows.1
Checking out files: 100% (258/258), done.
M5StickC @ 0df53b8 has been successfully installed!
Found 30 compatible libraries
Scanning dependencies...
Dependency Graph
|-- <SPI> 1.0
|-- <Wire> 1.0.1
|-- <ArduinoJson> 6.15.2
|-- <M5StickC> 0.2.0 #0df53b8
|   |-- <SPI> 1.0
|   |-- <Wire> 1.0.1
|   |-- <FS> 1.0
|   |-- <SPIFFS> 1.0
|   |   |-- <FS> 1.0
|-- <ESP32 BLE Arduino> 1.0.1
Building in release mode
Compiling .pio\build\m5stick-c\src\alpha.ino.cpp.o
...

...
Compiling .pio\build\m5stick-c\FrameworkArduino\wiring_pulse.c.o
Compiling .pio\build\m5stick-c\FrameworkArduino\wiring_shift.c.o
Archiving .pio\build\m5stick-c\libFrameworkArduino.a
Indexing .pio\build\m5stick-c\libFrameworkArduino.a
Linking .pio\build\m5stick-c\firmware.elf
Retrieving maximum program size .pio\build\m5stick-c\firmware.elf
Checking size .pio\build\m5stick-c\firmware.elf
Building .pio\build\m5stick-c\firmware.bin
esptool.py v2.6
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]  12.5% (used 40836 bytes from 327680 bytes)
Flash: [========  ]  82.7% (used 1083394 bytes from 1310720 bytes)
=========================================================== [SUCCESS] Took 996.93 seconds ===========================================================

Terminal will be reused by tasks, press any key to close it.
```
</details>

### Install USB to Serial Driver 
1. Download [CP210x Driver files](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/drivers/CP210x_VCP_Windows.zip)
2. Select appropriate executable depending on your OS Architecture
   - for 32-bit, choose `CP210xVCPInstaller_x86_vx.x.x.x.exe`
   - for 64-bit, choose `CP210xVCPInstaller_x64_vx.x.x.x.exe`

### Upload and Monitor
Ensure your device is connected and run `Upload and Monitor` from the `PROJECT TASKS` dropdown.


<details>
	<summary>Click to view sample output </summary>

```
> Executing task in folder trackstick_firmware: C:\Users\User\.platformio\penv\Scripts\platformio.exe run --target upload --target monitor <


Processing m5stick-c (platform: espressif32; board: m5stick-c; framework: arduino)
-----------------------------------------------------------------------------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/m5stick-c.html
PLATFORM: Espressif 32 1.12.3 > M5Stick-C
HARDWARE: ESP32 240MHz, 320KB RAM, 4MB Flash
DEBUG: Current (esp-prog) External (esp-prog, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
PACKAGES:
 - framework-arduinoespressif32 b92c58d
 - tool-esptoolpy 1.20600.0 (2.6.0)
 - tool-mkspiffs 2.230.0 (2.30)
 - toolchain-xtensa32 2.50200.80 (5.2.0)
Converting alpha.ino
LDF: Library Dependency Finder -> http://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 30 compatible libraries
Scanning dependencies...
Dependency Graph
|-- <SPI> 1.0
|-- <Wire> 1.0.1
|-- <ArduinoJson> 6.15.2
|-- <M5StickC> 0.2.0 #0df53b8
|   |-- <SPI> 1.0
|   |-- <Wire> 1.0.1
|   |-- <SPIFFS> 1.0
|   |   |-- <FS> 1.0
|   |-- <FS> 1.0
|-- <ESP32 BLE Arduino> 1.0.1
Building in release mode
Compiling .pio\build\m5stick-c\src\alpha.ino.cpp.o
Retrieving maximum program size .pio\build\m5stick-c\firmware.elf
Checking size .pio\build\m5stick-c\firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [=         ]  12.5% (used 40836 bytes from 327680 bytes)
Flash: [========  ]  82.7% (used 1083394 bytes from 1310720 bytes)
Configuring upload protocol...
AVAILABLE: esp-prog, espota, esptool, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa
CURRENT: upload_protocol = esptool
Looking for upload port...
Auto-detected: COM7
Uploading .pio\build\m5stick-c\firmware.bin
esptool.py v2.6
Serial port COM7
Connecting.....
Chip is ESP32-PICO-D4 (revision 1)
Features: WiFi, BT, Dual Core, 240MHz, Embedded Flash, VRef calibration in efuse, Coding Scheme None
MAC: d8:a0:1d:58:50:3c
Uploading stub...
Running stub...
Stub running...
Changing baud rate to 1500000
Changed.
Configuring flash size...
Auto-detected Flash size: 4MB
Compressed 17056 bytes to 11160...
Wrote 17056 bytes (11160 compressed) at 0x00001000 in 0.1 seconds (effective 1066.0 kbit/s)...
Hash of data verified.
Compressed 3072 bytes to 128...
Wrote 3072 bytes (128 compressed) at 0x00008000 in 0.0 seconds (effective 1638.4 kbit/s)...
Hash of data verified.
Compressed 8192 bytes to 47...
Wrote 8192 bytes (47 compressed) at 0x0000e000 in 0.0 seconds (effective 4369.1 kbit/s)...
Hash of data verified.
Compressed 1083632 bytes to 607676...
Wrote 1083632 bytes (607676 compressed) at 0x00010000 in 10.0 seconds (effective 869.6 kbit/s)...
Hash of data verified.

Leaving...
Hard resetting via RTS pin...
=========================================================== [SUCCESS] Took 34.59 seconds ===========================================================
--- Available filters and text transformations: colorize, debug, default, direct, esp32_exception_decoder, hexlify, log2file, nocontrol, printable, send_on_enter, time
--- More details at http://bit.ly/pio-monitor-filters
--- Miniterm on COM7  115200,8,N,1 ---
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
ets Jun  8 2016 00:22:57

rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 188777542, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0018,len:4
load:0x3fff001c,len:1044
load:0x40078000,len:10044
load:0x40080400,len:5872
entry 0x400806ac
[E][BLERemoteCharacteristic.cpp:275] retrieveDescriptors(): esp_ble_gattc_get_all_descr: Unknown
[E][BLERemoteCharacteristic.cpp:275] retrieveDescriptors(): esp_ble_gattc_get_all_descr: Unknown
CORRUPT HEAP: Bad head at 0x3ffe7f74. Expected 0xabba1234 got 0x3ffe7ff8
abort() was called at PC 0x40083659 on core 1

ELF file SHA256: 0000000000000000000000000000000000000000000000000000000000000000

Backtrace: 0x4008f804:0x3ffc93f0 0x4008fa81:0x3ffc9410 0x40083659:0x3ffc9430 0x4008377d:0x3ffc9460 0x400ede77:0x3ffc9480 0x400e8349:0x3ffc9740 0x400e08ac:0x3ffc9790 0x40095a85:0x3ffc97c0 0x40083d0e:0x3ffc97e0 0x40083599:0x3ffc9800 0x4000bec7:0x3ffc9820 0x40092d77:0x3ffc9840 0x400dd32f:0x3ffc9860 0x400da05f:0x3ffc9880 0x400da761:0x3ffc98a0 0x400dab4e:0x3ffc98c0 0x400dab6d:0x3ffc98e0 0x400d89aa:0x3ffc9900 0x400d3ae1:0x3ffc9930 0x400d3c09:0x3ffc9950 0x400d1f4d:0x3ffc9a40 0x400def59:0x3ffc9a60 0x40092e19:0x3ffc9a80

Rebooting...
ets Jun  8 2016 00:22:57

rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
configsip: 188777542, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:2
load:0x3fff0018,len:4
load:0x3fff001c,len:1044
load:0x40078000,len:10044
load:0x40080400,len:5872
entry 0x400806ac
```
</details>

Upon interacting with the Serial Monitor, you should see the following prompt:

```
This is an example of ESP-IDF console component.
Type 'help' to get the list of commands.
Use UP/DOWN arrows to navigate through command history.
Press TAB when typing command name to auto-complete.
esp32>
```

Upon entering `version` and hitting the <kbd>return</kbd> key, you should be provided with the model information as shown below:
```
esp32> version
IDF Version:v3.3.1-61-g367c3c09c
Chip info:
        model:ESP32
        cores:2
        feature:/802.11bgn/BLE/BT/Embedded-Flash:
        revision number:1
```

### Updating Libraries

Simply run `Update project libraries` from the `PROJECT TASKS` dropdown to keep the dependencies up-to-date.

```
> Executing task in folder trackstick_firmware: C:\Users\User\.platformio\penv\Scripts\platformio.exe lib update <

Library Storage: C:\Users\User\trackstick_firmware\.pio\libdeps\m5stick-c
Updating ArduinoJson                     @ 6.15.2         [Up-to-date]
Updating M5StickC                        @ 0df53b8        [Up-to-date]
```

### Additional References
- [PlatformIO ESP32 configuration](https://docs.platformio.org/en/latest/platforms/espressif32.html)