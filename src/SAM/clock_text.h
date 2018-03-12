#ifndef _CLOCK_TEXT_H_
#define _CLOCK_TEXT_H_

#include "rle_graphics.h"
#include "fonts/clock_font.c"

extern TFT_ST7735 tft;

#define font_width 59
#define font_height 15

void displayNumber(uint8_t chr, uint8_t x, uint8_t y)
{
  unsigned char const*cmp;
  
  // extract pointer to RLE encoded character 
  if (chr >= '0' && chr <='9')
  {
    cmp = clock_font_array[chr - '0'];
  } 
  else if (chr == ':')
    cmp = clock_font_array[10];
  else
    return; 

  //  set correct rotation (font is rotated 90deg for better RLE compression)
  tft.setRotation(0);

  // render character 
  display_RLE_4BITGRAY(cmp, x, y, font_width, font_height);
  
}



#endif
