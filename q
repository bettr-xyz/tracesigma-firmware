[1mdiff --git a/alpha/hal.cpp b/alpha/hal.cpp[m
[1mindex 9fc72a9..e5d3907 100644[m
[1m--- a/alpha/hal.cpp[m
[1m+++ b/alpha/hal.cpp[m
[36m@@ -304,6 +304,16 @@[m [mvoid _TS_HAL::power_set_mic(bool enabled)[m
   EXIT_CRITICAL;[m
 }[m
 [m
[32m+[m[32muint8_t _TS_HAL::power_get_batt_level(){[m
[32m+[m[32m  long level;[m
[32m+[m[32m  ENTER_CRITICAL;[m
[32m+[m[32m#ifdef HAL_M5STICK_C[m
[32m+[m[32m  level = constrain(map(M5.Axp.GetVbatData() * 1.1, 3100, 4000, 0, 100), 0, 100);[m
[32m+[m[32m#endif[m
[32m+[m[32m  EXIT_CRITICAL;[m
[32m+[m[32m  return level;[m
[32m+[m[32m}[m
[32m+[m
 //[m
 // Common logging functions[m
 //[m
[1mdiff --git a/alpha/hal.h b/alpha/hal.h[m
[1mindex c76656a..75edbdb 100644[m
[1m--- a/alpha/hal.h[m
[1m+++ b/alpha/hal.h[m
[36m@@ -121,6 +121,7 @@[m [mclass _TS_HAL[m
     void power_off();[m
     void reset();[m
     void power_set_mic(bool);[m
[32m+[m[32m    uint8_t power_get_batt_level();[m
 [m
     [m
     //[m
[1mdiff --git a/alpha/ui.cpp b/alpha/ui.cpp[m
[1mindex 4fa1911..9fae3a6 100644[m
[1m--- a/alpha/ui.cpp[m
[1m+++ b/alpha/ui.cpp[m
[36m@@ -61,6 +61,7 @@[m [mvoid _TS_UI::task(void* parameter)[m
       // TODO: does not work with F()[m
       TS_HAL.lcd_printf("Date: %04d-%02d-%02d\n",     datetime.year, datetime.month, datetime.day);[m
       TS_HAL.lcd_printf("Time: %02d : %02d : %02d\n", datetime.hour, datetime.minute, datetime.second);[m
[32m+[m[32m      TS_HAL.lcd_printf("Battery: %d%%", TS_HAL.power_get_batt_level(), NULL, NULL);[m
     }[m
 [m
     // stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);[m
[36m@@ -104,4 +105,3 @@[m [mvoid _TS_UI::task(void* parameter)[m
     TS_HAL.sleep(TS_SleepMode::Task, 250);[m
   }[m
 }[m
[31m-[m
