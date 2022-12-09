/**
   Helper.cpp
   @autor    Bruno Merz

   @version  1.0
   @created  17.03.2021
   @updated  

  Helper class with static methods

*/


//#define myDEBUG
#include "Configuration.h"
#include "Helper.h"
#include "MySpiffs.h"

static MySpiffs *myfs=0;


void Helper::writeState(const char* text) {
  char buffer[100];
  if(text) {
    strcpy_P(buffer, text);
    if(!myfs)
        myfs = MySpiffs::getInstance();
    myfs->writeLog(buffer);
    DEBUG_PRINT(buffer);
  }
}


void Helper::writeState(const char* text, char *val) {
  writeState(text);
  if(val) {
    if(!myfs)
        myfs = MySpiffs::getInstance();
    myfs->writeLog(val);
    DEBUG_PRINT(val);
  }
}





void Helper::writeState(const char* text, String val) {
  writeState(text, (char *)val.c_str());
}


void Helper::writeState(const char* text, int val) {
  char tmp[10];
  itoa(val, tmp, 10);
  writeState(text, tmp);
}
