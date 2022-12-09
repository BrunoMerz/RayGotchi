/*
   Settings.cpp
   @autor    Bruno Merz

   @version  2.0
   @created  30.12.2019

*/

//#define myDEBUG


#include "Settings.h"

StaticJsonDocument<JSON_ARRAY_SIZE(1)+JSON_OBJECT_SIZE(100)> jsonDoc;

// Initialize Setting instance pointer
Settings* Settings::instance = 0;


/**
   Constructor
*/
Settings::Settings(void) {
// nothing found so far to initialize
}

/**
   Destructor, ends LittleFS
*/
Settings::~Settings(void) {
  writeSettings();
}


// Public class method which returns a pointer to the settings object (singleton pattern)
Settings *Settings::getInstance() {
    if (!instance)
    {
        instance = new Settings();
    }
    return instance;
}

// Read all settings from file into Json object
bool Settings::readSettings(String filename) {
  _filename = filename;
  File jsonFile;
  //read Json File
  DEBUG_PRINTLN("readJson: "+_filename);
  if (LittleFS.exists(_filename)) {
    //file exists, reading and loading
    jsonFile = LittleFS.open(_filename, "r");
    if (jsonFile) {
      deserializeJson(jsonDoc, jsonFile);
      jsonFile.close();
#if defined(myDEBUG)
        serializeJson(jsonDoc, DBG_OUTPUT_PORT);
        DEBUG_PRINTLN("");
#endif
    } else {
        DEBUG_PRINT("Failed to load json config: ");
        return false;
    }
  } else {
    // create JsonFile
    DEBUG_PRINTLN("Creating " + _filename);
    jsonFile = LittleFS.open(_filename, "w");
    jsonFile.close();
  }

  return true;
}

/**
   Write JSON settings to LittleFS
*/
bool Settings::writeSettings(void) {
    DEBUG_PRINTLN("writeSettings: "+_filename);
    File configFile = LittleFS.open(_filename, "w");
    if (configFile) {
#if defined(myDEBUG)
      serializeJson(jsonDoc, DBG_OUTPUT_PORT);
      DEBUG_PRINTLN();
#endif
      serializeJson(jsonDoc, configFile);
      configFile.println();
      DEBUG_PRINTLN("Closing configFile");
      configFile.flush();
      configFile.close();
      return true;
    } else {
      DEBUG_PRINTLN("failed to open config file for writing");
      return false;
    }
}

/**
   set all settings from a string
*/
void Settings::set(String settings) {
  DEBUG_PRINTLN("set: '" + settings + "'");
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, settings);
  JsonObject root = doc.as<JsonObject>();

  for (JsonPair kv : root) {
    String settingName(kv.key().c_str());
    String settingValue=kv.value().as<String>();
    set(settingName, settingValue);

  }

  writeSettings();
}


/**
   set a setting Name=Value
*/
void Settings::set(String settingName, String settingValue) {
   jsonDoc[settingName] = settingValue;
}


/**
   set a setting Name=Value
*/
void Settings::set(String settingName, int settingValue) {
   jsonDoc[settingName] = settingValue;
}


/**
   set rgb setting Name=Value
*/
void Settings::setColor(String settingName, COLOR_T color) {
  char rgbValue[10];
  sprintf(rgbValue,"#%.6x",color);
  jsonDoc[settingName] = rgbValue;

}



/**
   get value for a specific name
*/
String Settings::get(String settingName) {
  String _settingValue;
  JsonVariant jv = jsonDoc[settingName];
  if (jv.isNull())
    _settingValue = "";
  else
    _settingValue = jv.as<String>();
    
  DEBUG_PRINTLN("getSetting: "+settingName+"='"+_settingValue+"'");
  return _settingValue;
}


/**
   get char* value for a specific name
*/
int Settings::getChar(String settingName, char *value, int lng) {
  JsonVariant jv = jsonDoc[settingName];
  if (jv.isNull())
    *value = '\0';
  else
    strncpy(value, jv.as<String>().c_str(), lng);
  return strlen(value);
}


/**
   get int value for a specific name
*/
int Settings::getInt(String settingName) {
  JsonVariant jv = jsonDoc[settingName];
  if (jv.isNull())
    return -1;
  else
    return strtol(jv.as<String>().c_str(), NULL, 0);
}


/**
   get rgb value for a specific name
*/
COLOR_T Settings::getRGB(String settingName) {
  String _settingValue;
  COLOR_T c;
  JsonVariant jv = jsonDoc[settingName];
  if (jv.isNull())
    c = (COLOR_T) 0xffffff;
  else {
    _settingValue = jv.as<String>();
    if(_settingValue.startsWith("#"))
      c = (COLOR_T) strtol( &_settingValue.c_str()[1], NULL, 16);   // #FFAABB
    else
      c = (COLOR_T) strtol( _settingValue.c_str(), NULL, 0);        // 0xFFAABB or 16755387
  }

  return c;
}


/**
   get boolean value for a specific name
*/
bool Settings::getBool(String settingName) {
  String _settingValue;
  bool ret=false;
  JsonVariant jv = jsonDoc[settingName];
  if (!jv.isNull()) {
    _settingValue = jv.as<String>();
    if (_settingValue.equalsIgnoreCase("true"))
      ret=true;
    else if(_settingValue.equalsIgnoreCase("on"))
      ret=true;
    else if (_settingValue.equals("1"))
      ret=true;
  }
  DEBUG_PRINTF("getBoolSetting: %s=%d\n",settingName.c_str(), ret);
  return ret;
}


/**
   get double value for a specific name
*/
double Settings::getDouble(String settingName) {
  JsonVariant jv = jsonDoc[settingName];
  if (jv.isNull())
      return -1;
  else
    return jv.as<String>().toDouble();
}


/**
   get all settings
*/
String Settings::getAll(void) {
  _settings = "";
  size_t numBytes = serializeJson(jsonDoc, _settings);
  if (numBytes)
    DEBUG_PRINTLN("getAll spiffs: '" + _settings + "'");
  else {
    DEBUG_PRINTLN("getAll spiffs: Failed to serialize jsonDoc");
  }
  return _settings;
}


/**
   clear value for a specific name
*/
void Settings::remove(String settingName) {
  jsonDoc.remove(settingName);
}
