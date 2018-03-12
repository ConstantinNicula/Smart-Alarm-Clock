#ifndef _AATEXT_H_
#define _AATEXT_H_

#include "rle_graphics.h"

#include "fonts/Font.c"
#include <avr/pgmspace.h>


#define general_font_width  18
#define general_font_height  10

extern TFT_ST7735 tft;

// returns font 
uint8_t  displayCharacter(uint8_t chr, uint8_t x, uint8_t y)
{
  // extract pointer to RLE encoded character 
  unsigned char const*cmp = Font_array[chr];

  //  set correct rotation (font is rotated 90deg for better RLE compression)
  tft.setRotation(0);


  // first two bytes indicate font width
  uint8_t width, height;
  width = pgm_read_byte_near(cmp);
  cmp++;
  height = pgm_read_byte_near(cmp);
  cmp++;

  // render character 
  display_RLE_4BITGRAY(cmp, x, y, width, height);

  return height;
}
uint8_t printText(const char* string, uint8_t x, uint8_t y)
{
  uint8_t len = strlen(string);
  uint8_t text_width  = 0;
  for(uint8_t i = 0; i<len; i++)
  {
     uint8_t char_width = displayCharacter(string[i], x, y);
     y = y + char_width;
     text_width += char_width;
  }

  return text_width;
}

uint8_t computeTextWidth(const char* string)
{
  uint8_t len = strlen(string);
  uint8_t text_width  = 0;
  
  for(uint8_t i = 0; i<len; i++)
  {
     uint8_t char_width =  pgm_read_byte_near(Font_array[string[i]]+1);
     text_width += char_width;
  }

  return text_width;
}

#endif

