#pragma once

#include <Arduino.h>

#define CONFIG_IDF_TARGET_ESP32 1
#if CONFIG_IDF_TARGET_ESP32
  #define LCD_MOSI 18
  #define LCD_MISO -1
  #define LCD_SCK 5
  #define LCD_CS 22
  #define LCD_RST 19
  #define LCD_DC 21
  #define LCD_BL 23
  #define LCD_SCL_T 15
  #define LCD_SDA_T 16
  #define LCD_RST_T 12
  #define LCD_INT_T 26
#endif
