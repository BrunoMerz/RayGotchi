/**
   MyTft.cpp
   @autor    Bruno Merz

   @version  1.0
   @created  29.11.2021

*/

//#define myDEBUG
#include "Configuration.h"


#include "MyTft.h"

void MyTft::clearMainCanvas(void) {
  fillRect(0, 0, _mainCanvasWidth, _mainCanvasHeight, TFT_BLACK);
}

void MyTft::clearStateCanvas(void) {
  fillRect(0, _mainCanvasHeight, _mainCanvasWidth, fontHeight(STD_FONT), TFT_BLACK);
}


void MyTft::drawStateLine(String text) {
  clearStateCanvas();
  drawString(text,0,_stateLineYPos,STD_FONT);
}
