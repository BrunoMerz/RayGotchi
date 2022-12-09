/*
   MyWifi.h
   @autor    Bruno Merz

   @version  2.0
   @created  30.12.2019
 
*/

#pragma once

#include "Settings.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#define ESP_WPS_MODE      WPS_TYPE_PBC
#define ESP_MANUFACTURER  "ESPRESSIF"
#define ESP_MODEL_NUMBER  "ESP32"
#define ESP_MODEL_NAME    "ESPRESSIF IOT"
#define ESP_DEVICE_NAME   "ESP STATION"
#endif



#define WIFITIMEOUT 60
#define WPSTIMEOUT  30


class ESPAsync_WiFiManager;

class MyWifi {
  public:
    static MyWifi* getInstance();

    void init(void);
    static void saveConfigCallback (void);
    static void configModeCallback (ESPAsync_WiFiManager *myWiFiManager);
    bool myStartWPS(void);
    void connect(String ssid, String pass);
    void doReset(void);
    String getIP(void);

  private:
    MyWifi(void);
    ~MyWifi(void);

    static MyWifi *instance;

};
