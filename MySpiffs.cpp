/*
   MySpiffs.cpp
   @autor    Bruno Merz

   @version  2.0
   @created  30.12.2019

*/

#include <FS.h>
#include <LITTLEFS.h>

//#define myDEBUG
#include "Configuration.h"
#include "Helper.h"
#include "MySpiffs.h"


MySpiffs* MySpiffs::instance = 0;

MySpiffs::MySpiffs() {
  fsUploadFile = (File)NULL;
  _reloadResource = true;
}

MySpiffs::~MySpiffs() {
  LittleFS.end();
}

MySpiffs *MySpiffs::getInstance() {
  if (!instance)
  {
    instance = new MySpiffs();
  }
  return instance;
}


/**
   removes a file from LittleFS
*/
void MySpiffs::remove(String filename) {
  LittleFS.remove(filename);
}


/**
   open LittleFS file
*/
File MySpiffs::openFile(String filename, char *mode) {
  DEBUG_PRINTLN("openFile: " + filename);
  fsUploadFile = LittleFS.open(filename, mode);
  if (fsUploadFile)
    DEBUG_PRINTLN("openFile: ok");
  return fsUploadFile;
}


/**
   write LittleFS file
*/
void MySpiffs::writeFile(const uint8_t *buf, size_t len) {
  //DEBUG_PRINTLN("writeFile: len"+len);
  if (fsUploadFile)
    fsUploadFile.write(buf, len);
}

/**
   write LittleFS file
*/
void MySpiffs::writeFile(char *buf) {
  //DEBUG_PRINTLN("writeFile: len"+len);
  if (fsUploadFile)
    fsUploadFile.write((const uint8_t*)buf, strlen(buf));
}


/**
   close LittleFS file
*/
void MySpiffs::closeFile(void) {
  DEBUG_PRINTLN("closeFile:");
  if (fsUploadFile) {
    fsUploadFile.flush();
    fsUploadFile.close();
    fsUploadFile = (File)NULL;
  }
}


/**
   read file from LittleFS
*/
size_t MySpiffs::readBytes(uint8_t *buffer, int len) {
  DEBUG_PRINT("readBytes: len=");
  DEBUG_PRINTLN(len);
  size_t l = 0;
  if (fsUploadFile) {
    l = fsUploadFile.readBytes((char *)buffer, len);
  }
  return l;
}


/**
   read file from LittleFS
*/
int MySpiffs::readFile(String filename, char *buffer) {
  DEBUG_PRINTLN("readFile: start '" + filename + "'");
  if (LittleFS.exists(filename)) {
    //file exists, reading and loading
    DEBUG_PRINTLN("readFile: reading " + filename);
    File fileObj = LittleFS.open(filename, "r");
    if (fileObj) {
      DEBUG_PRINTLN("readFile: File opened!");
      _fileSize = fileObj.size();
      DEBUG_PRINT("#bytes=");
      DEBUG_PRINTLN(_fileSize);
      int x = fileObj.readBytes(buffer, _fileSize);
      DEBUG_PRINT("readFile: readBytes result=");
      DEBUG_PRINTLN(x);
      *(buffer + _fileSize) = '\0';
      fileObj.close();
    }
  } else {
    DEBUG_PRINTLN("readFile: no file in FS");
    *buffer   = '\0';      // file not found
    _fileSize = 0;
  }
  return _fileSize;
}

int MySpiffs::readFileS(String filename, String *buffer) {
  DEBUG_PRINTLN("readFileS: start '" + filename + "'");
  if (LittleFS.exists(filename)) {
    //file exists, reading and loading
    DEBUG_PRINTLN("readFileS: reading " + filename);
    File fileObj = LittleFS.open(filename, "r");
    if (fileObj) {
      DEBUG_PRINTLN("readFileS: File opened!");
      *buffer = fileObj.readString();
      fileObj.close();
    }
  } else {
    DEBUG_PRINTLN("readFileS: no file in FS");
    *buffer = ""; // file not found
  }
  return buffer->length();
}


/**
   format LittleFS
*/
void MySpiffs::format(void) {
  bool formatted = LittleFS.format();
  if (formatted) {
    DEBUG_PRINTLN("\n\nSuccess formatting");
  } else {
    DEBUG_PRINTLN("\n\nError formatting");
  }
}


/**
   file exists?
*/
bool MySpiffs::exists(String filename) {
  return LittleFS.exists(filename);
}



/**
   getHtmlFilename checks if Update.htm is required
*/
String MySpiffs::getHtmlFilename(String html, bool checkUpdate) {
  if (checkUpdate && LittleFS.exists("/result.html") && LittleFS.exists("/update.html")) {
    return ("/update.html");
  }
  else {
    return "/" + html + ".html";
  }
}


/**
   read file size from LittleFS
*/
int MySpiffs::fileSize(String filename) {
  int fs = -1;
  if (LittleFS.exists(filename)) {
    File fileObj = LittleFS.open(filename, "r");
    if (fileObj) {
      fs = fileObj.size();
      fileObj.close();
    }
  }
  return fs;
}


/**
   Log Funktions
*/
void MySpiffs::writeLog(int32_t val, boolean withDateTime) {
  char dest[100];
  sprintf(dest, "%d", val);
  writeLog(dest, withDateTime);
}

void MySpiffs::writeLog(const __FlashStringHelper *src, boolean withDateTime) {
  char dest[100];
  strncpy_P(dest, (char *)src, sizeof(dest));
  writeLog(dest, withDateTime);
}


void MySpiffs::writeLog(char *txt, boolean withDateTime) {
  logHandle = LittleFS.open(LOG_FILE, "a");

  if (logHandle) {
    if (logHandle.size() > 5000) {
      logHandle.close();
      LittleFS.rename(LOG_FILE, LOG_FILE_OLD);
      logHandle = LittleFS.open(LOG_FILE, "w");
    }
    if (withDateTime) {
      //String t =  "xxxxx";//mytime.getDate();
      char *buffer;
      time_t now = time(nullptr);
      buffer = ctime(&now);
      String t(buffer);

      logHandle.print(t.substring(0, t.length() - 1).c_str());
      logHandle.print(": ");

      now = time(nullptr);
      struct tm *lt = localtime(&now);
      strftime(buffer, sizeof(buffer), "%H:%M:%S", lt);

      logHandle.print(buffer);
      logHandle.print(": ");
    }
    logHandle.print(txt);
    logHandle.flush();
    logHandle.close();
  }
}

uint32_t MySpiffs::getFreeSpace(void) {
#if defined(ESP8266)
  FSInfo fs_info;
  LittleFS.info(fs_info);
  return fs_info.totalBytes - fs_info.usedBytes;
#else
  return LittleFS.totalBytes() - LittleFS.usedBytes();
#endif
}


/**
   reloadResource
*/
void MySpiffs::reloadResource(void) {
  _reloadResource = true;
}

/**
   cleanup unneccessary files
*/
void MySpiffs::cleanup(void) {
#if defined(ESP32)
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while (file) {
    String fn(file.name());
    if (fn.endsWith(".tar") || fn.endsWith(".bin") || fn.endsWith(".txt"))
      LittleFS.remove(file.name());
    file = root.openNextFile();
  }
#else
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String fn(dir.fileName());
    if (fn.endsWith(".tar") || fn.endsWith(".bin") || fn.endsWith(".txt"))
      LittleFS.remove(dir.fileName());
  }
#endif
}

/**
   get state
*/
String MySpiffs::getState(void) {
  File file;
  char txt[100];
  int total = 0;

  writeLog(F("\n\nFILES:\n"), false);

  File rt = LittleFS.open("/");
  file = rt.openNextFile();
  while (file) {
    sprintf(txt, "%-20s", file.name());
    writeLog(txt, false);
    sprintf(txt, "%.6d", file.size());
    writeLog(F(", size="), false);
    writeLog(txt, false);
    
    time_t lw = file.getLastWrite();
    struct tm * tmstruct = localtime(&lw);
    sprintf(txt, "%d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    writeLog(F(", last write="), false);
    writeLog(txt, false);
    writeLog(F("\n"),false);

    total += file.size();
    file = rt.openNextFile();
  }
  writeLog(F("\nAll files total size="), false);
  writeLog(total, false);
  writeLog(F("\nFilesystem total bytes="), false);
  writeLog(LittleFS.totalBytes(), false);
  writeLog(F("\nFilesystem used bytes="), false);
  writeLog(LittleFS.usedBytes(), false);
  writeLog(F("\n"),false);
  
  return "";
}
