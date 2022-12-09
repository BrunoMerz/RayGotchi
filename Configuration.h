/*
   Configuration.h
   @autor    Bruno Merz

   @version  1.0
   @created  20.09.2022

   Configuration
*/

#pragma once


const char compile_date[] = __DATE__ " " __TIME__;

enum EXEC_CALLBACK {
  WIFI_OK,
  WIFI_WPS,
  WIFI_HOTSPOT,
  WIFI_CONNECTING,
  WIFI_TIMEOUT,
  WIFI_RESET,
  SETTINGS_UPDATE,
  FW_OR_TAR,
  UPLOAD,
  UPLOAD_LENGTH,
  UPLOAD_FW_OK,
  GET_STATE,
  STATE_TEXT,
};

#define COLOR_T uint32_t

#if defined(myDEBUG)
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINT2(x, y) Serial.print(x, y)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTLN2(x, y) Serial.println(x, y)
#define DEBUG_PRINTF   Serial.printf
#define DEBUG_FLUSH() Serial.flush()
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINT2(x, y)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTLN2(x, y)
#define DEBUG_PRINTF
#define DEBUG_FLUSH()
#endif

#define DBG_OUTPUT_PORT Serial
#define LITTLEFS_AUDIO

#define COLOR_T uint32_t

#define SETTINGS_FILENAME "/cfg.json"
#define CONFIGPORTAL_SSID "RayGotchi"

#define TEXTPLAIN "text/plain"

#define LOG_FILE            F("/logfile.log")   // Name of logfile
#define LOG_FILE_OLD        F("/logfile1.log")  // Name of old logfile
#define INFO_FILE           F("/info.txt")      // Info Text für die Installation

#define BUFSIZE 500

#define GOTCHI "I'm Gotchi #X"

#define C_SERVICE "ffc0"
#define C_CHAR1   "ffc1"
#define C_CHAR2   "ffc2"

#define TFT_W 240
#define TFT_H 240
#define STD_FONT 4

#define WIFI_RESET_BUTTON  33
