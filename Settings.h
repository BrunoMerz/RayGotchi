/*
   Settings.h
   @autor    Bruno Merz

   @version  2.0
   @created  30.12.2019
 */

#pragma once

#include <ArduinoJson.h>
#include "Configuration.h"

#define USE_LittleFS
#include <FS.h>
#include <LittleFS.h>

class Settings {
  public:
    static Settings* getInstance();

    bool    writeSettings(void);
    bool    readSettings(String filename);
 
    void    set(String settings);
    void    set(String settingName, String settingValue);
    void    set(String settingName, int settingValue);
    void    setColor(String settingName, COLOR_T settingValue);
    String  get(String settingName);
    int     getInt(String settingName);
    int     getChar(String settingName, char *value, int lng);
    bool    getBool(String settingName);
    double  getDouble(String settingName);
    COLOR_T getRGB(String settingName);
    String  getAll(void);
    void    remove(String settingName);

private:
    Settings(void);
    ~Settings(void);

    static Settings *instance;

    String  _filename;
    String _settings;

};
