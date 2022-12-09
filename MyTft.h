/**
   MyTft.h
   @autor    Bruno Merz

   @version  1.0
   @created  29.11.2021

*/

#pragma once

#include <TFT_eSPI.h>
#include <SPI.h>

class MyTft: public TFT_eSPI {
  public:
    MyTft(void): TFT_eSPI(TFT_W,TFT_H) {
      _mainCanvasWidth = width();
      _mainCanvasHeight = height()-fontHeight(STD_FONT);
      _stateLineYPos = _mainCanvasHeight;
      
    };
    void drawStateLine(String text);
    void clearMainCanvas(void);
    void clearStateCanvas(void);
  private:
    uint16_t _stateLineYPos;
    uint16_t _mainCanvasWidth;
    uint16_t _mainCanvasHeight;
    
};
