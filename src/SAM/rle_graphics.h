#ifndef _RLE_GRAPHICS_H_
#define _RLE_GRAPHICS_H_

extern TFT_ST7735 tft;

#define BLACK TFT_BLACK

uint16_t gray_lut[] = {
    0x0000, 0x1082, 0x2104, 0x3186, 0x4208, 0x4228, 0x52AA, 0x632C,0x73AE, 0x8430, 0x94B2, 0xB596, 0xC618, 0xD69A, 
    0xE71C, 0xF79E, 0xFFFF 
};

void display_RLE_4BITGRAY( unsigned char const*cmp, int x, int y,  uint8_t width, uint8_t height)
{
  uint16_t outp = 0, endp = width*height;
  //Serial.println("RLE_BLIT!");
  // set address window 
  tft.setAddrWindow(x, y, x + (width - 1), y + (height - 1));

  // while we have pixles left 
  while(outp <  endp)
  {
       uint8_t val = pgm_read_byte_near(cmp);
       cmp++;
       if (val & 0x80)
       {
          // the next !val + 1 words are unique
          val = ~(val) + 1;
          for (uint16_t i = 0; i < val; i++)
          {
              // each sequence contains two pixels.
              uint8_t seq =  pgm_read_byte_near(cmp);
              cmp++;
              
              // push first pixel color 
              tft.pushColor(gray_lut[(seq & 0xf0) >> 4]);
              tft.pushColor(gray_lut[(seq & 0x0f)]);
              outp+=2;
          }  
       }
       else 
       {
          // val indicates the number this pixle sequence repeats.
          uint16_t color =  pgm_read_byte_near(cmp);
          cmp++;
          for (uint16_t i = 0; i < val; i++)
          {
              tft.pushColor(gray_lut[(color & 0xf0) >> 4]);
              tft.pushColor(gray_lut[(color & 0x0f)]);
              outp+=2;
          }  
        
       }    
  }


}

#endif

