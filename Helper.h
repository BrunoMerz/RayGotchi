/**
   Helper.h
   @autor    Bruno Merz

   @version  1.0
   @created  17.03.2021
 

  Helper class with static methods

*/

#pragma once

#include <Arduino.h>    // wg String

class Helper {
  public:
    static void writeState(const char* text);
    static void writeState(const char* text, String val);
    static void writeState(const char* text, char *val);
    static void writeState(const char* text, int val);
 
};
