#ifndef _WIDGET_H_
#define _WIDGET_H_

#include "aatext.h"
#include "bitmaps.h"
#include "rle_graphics.h"
#include "clock_text.h"



/* Definition of Clock widget */

#define REDRAW_CLOCK (1<<7)
#define SHOW_HOUR (1<<2)
#define SHOW_MIN (1<<1)
#define SHOW_SEC (1<<0)

#define INT2CHAR(x) ((char) ((x) + 48))

void displayClockWidget(uint8_t x, uint8_t y, uint8_t hour, uint8_t min, uint8_t sec, uint8_t disp_mask )
{
  static uint8_t prev_hour = 0, prev_min = 0 , prev_sec = 0;
  static uint8_t prev_disp_mask = 0;


  // if MSB of disp_mask is set, redraw the whole widget.

  // for each segment(hour/min/sec)
  if(disp_mask & REDRAW_CLOCK)
  {
    displayNumber(INT2CHAR(hour/10), x, y);
    displayNumber(INT2CHAR(hour%10), x, y + font_height);
    displayNumber(':', x, y + font_height * 2);
    displayNumber(INT2CHAR(min/10), x, y + font_height * 3);
    displayNumber(INT2CHAR(min%10), x, y + font_height * 4);
    displayNumber(':', x, y + font_height * 5);
    displayNumber(INT2CHAR(sec/10), x, y + font_height * 6);
    displayNumber(INT2CHAR(sec%10), x, y + font_height * 7);

    // store data and exit 

    prev_hour = hour;
    prev_min = min;
    prev_sec = sec;
 
    prev_disp_mask = disp_mask | SHOW_HOUR | SHOW_MIN | SHOW_SEC;
    return;
  }

  // if show hour bit is set
  if(disp_mask & SHOW_HOUR)
  {
    // check hour
    if (hour/10 != prev_hour/10 || !(prev_disp_mask & SHOW_HOUR) ) // if it differs display new character
    {
        displayNumber(INT2CHAR(hour/10), x, y);
        displayNumber(INT2CHAR(hour%10), x, y + font_height);
    } 
    else if (hour % 10 != prev_hour % 10 )
    {
       displayNumber(INT2CHAR(hour%10), x, y + font_height);
    } 
  }
  else 
  {
      if(prev_disp_mask & SHOW_HOUR) // do not redraw if not necessary
      {
        tft.setRotation(0);
        // clear area
        tft.fillRect (x, y,font_width, 2*font_height, BLACK);
      }
  }


  // if show minute is set 
  if(disp_mask & SHOW_MIN)
  {
    // check minutes
    if (min/10 != prev_min/10 || !(prev_disp_mask & SHOW_MIN)) // if it differs display new character
    {
        displayNumber(INT2CHAR(min/10), x, y + 3*font_height);
        displayNumber(INT2CHAR(min%10), x, y + 4*font_height);
    } 
    else if (min % 10 != prev_min % 10 )
    {
       displayNumber(INT2CHAR(min%10), x, y + 4*font_height);
    } 
  }
  else 
  {
    if(prev_disp_mask & SHOW_MIN) // do not redraw if not necessary
    {
      tft.setRotation(0);
      // clear area
      tft.fillRect (x, y + 3*font_height, font_width, 2*font_height, BLACK);
    }
  }


  // if show sec is set
  if(disp_mask & SHOW_SEC)
  {
    // check seconds
    if (sec/10 != prev_sec/10 || !(prev_disp_mask & SHOW_SEC)) // if it differs display new character
    {
        displayNumber(INT2CHAR(sec/10), x, y + 6*font_height);
        displayNumber(INT2CHAR(sec%10), x, y + 7*font_height);
    } 
    else if (sec % 10 != prev_sec % 10 )
    {
       displayNumber(INT2CHAR(sec%10), x, y + 7*font_height);
    } 
  }
  else
  {
    if(prev_disp_mask & SHOW_SEC) // do not redraw if not necessary
    {
      tft.setRotation(0);
      // clear area
      tft.fillRect (x, y + 6*font_height, font_width, 2*font_height, BLACK);
    }
   }
    

  // update new values 
  prev_hour = hour;
  prev_min = min;
  prev_sec = sec;
  
  
  prev_disp_mask = disp_mask;
}


/* Definition of date widget */

static const char month_desc[13][5] = {
  "","Jan ", "Feb ", "Mar ", "Apr ", "May ", "June", "July", "Aug ", "Sept", "Oct ", "Nov ", "Dec " 
  };

static char days_of_the_week[8][6] = {
  " ", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat", "Sun" 
  };

#define REDRAW_DATE (1<<7)
#define SHOW_MONTH (1<<2)
#define SHOW_DAY (1<<1)
#define SHOW_YEAR (1<<0)

void displayDateWidget(uint8_t x, uint8_t y, uint16_t year, uint8_t month, uint8_t day, uint8_t day_index, uint8_t disp_mask) 
{
  static uint8_t prev_month, prev_day;
  static uint16_t prev_year;
  static uint8_t prev_disp_mask = 0;
  static char buf[10];

  // redraw all and exit 
  if(disp_mask & REDRAW_DATE)
  {
    // print day of week text
    y += printText(days_of_the_week[day_index], x, y);
    y += printText(", ", x, y);

    // print month
    y += printText( month_desc[month], x, y);
    y += printText(" ", x, y);

    // print day
    itoa(day, buf, 10);
    y += printText(buf, x, y);
    y += printText(", ", x, y);

    // print year
    itoa(year, buf, 10);
    y += printText(buf, x, y);

    // clear rest of line 
    tft.fillRect(x, y, 19, (160-y), BLACK);


    prev_month = month;
    prev_year = year;
    prev_day = day;

    prev_disp_mask = disp_mask | SHOW_YEAR | SHOW_DAY | SHOW_MONTH;

    return;
  }

  // if nothing changed exit 
  if (disp_mask == prev_disp_mask && month == prev_month && year == prev_year && day == prev_day)
    return; 

  if(disp_mask != 0x0)
  {
  // at least one item changed refresh whole line 
  y += printText(days_of_the_week[day_index], x, y);
  y += printText(", ", x, y);

  // if we need to display the month do so.
  if(disp_mask & SHOW_MONTH)
  {
    y += printText( month_desc[month], x, y);
  }
  else // print padding 
  {
    uint8_t width = computeTextWidth(month_desc[month]);
    tft.fillRect(x, y, 19, width, BLACK);
    y += width;
  }
  y += printText(" ", x, y);
  
  // display day
  if(disp_mask & SHOW_DAY)
  {
    itoa(day, buf, 10);
    y += printText(buf, x, y);
  }
  else // print 2 spaces padding
  {
    itoa(day, buf, 10);
    uint8_t width = computeTextWidth(buf);
    tft.fillRect(x, y, 19, width , BLACK);
    y += width;
  }
  
  y += printText(", ", x, y);


  // display year 
  if(disp_mask & SHOW_YEAR)
  {
    itoa(year, buf, 10);
    y += printText(buf, x, y);
  }
  else // print 4 spaces padding
  {
    itoa(year, buf, 10);
    uint8_t width = computeTextWidth(buf);
    tft.fillRect(x, y, 19, width , BLACK);
    y += width;
  }
  
  tft.fillRect(x, y, 19, (160-y), BLACK);
  }
  else 
  {
    tft.fillRect(x, y, 19, (160-y), BLACK);
  }
  

  // store new data 
  prev_day = day;
  prev_month = month;
  prev_year = year;


  prev_disp_mask = disp_mask;
  
}


/*Definiton of temmp widget */
//static uint8_t temp_icon_x = 95;
//static uint8_t temp_icon_y = 78; 


#define REDRAW_TEMP (1<<7)
#define SHOW_TEMP_ICON (1<<1)
#define SHOW_TEMP_TEXT (1<<0)



void displayTempWidget(uint8_t x, uint8_t y, int8_t temp, uint8_t disp_mask )
{
  static int8_t text_offest_x = temp_icon_height -32;
  static int8_t text_offset_y = temp_icon_width + 4;
  static uint8_t prev_disp_mask = 0;
  static int8_t prev_temp = 0;
  char buf[8];
  
  // the icon has a non standard rotation, addapt coordinates
  uint8_t x_i, y_i;
  x_i = y;
  y_i = 128 - x -  temp_icon_height;
  
  // update text start position
  y += text_offset_y;
  x += text_offest_x;

    
  if (disp_mask & REDRAW_TEMP)
  {
    //  set correct rotation 
    tft.setRotation(1);
    
    // display icon 
    display_RLE_4BITGRAY(&image_data_temp[0], x_i , y_i, temp_icon_width, temp_icon_height);

    // display text 

    itoa(temp, buf, 10);
    y += printText(buf, x, y);
    y += printText("?C", x, y);


    prev_temp = temp;
    prev_disp_mask = disp_mask | SHOW_TEMP_ICON | SHOW_TEMP_TEXT;  

    //Serial.print("Redraw!");
    return;
  }


  // update icon if necessary 
  if (disp_mask & SHOW_TEMP_ICON)
  {
    if(!(prev_disp_mask & SHOW_TEMP_ICON))
    {
      //  set correct rotation 
      tft.setRotation(1);
      
      // display icon 
      display_RLE_4BITGRAY(&image_data_temp[0], x_i , y_i, temp_icon_width, temp_icon_height);
      //Serial.println("New Icon");
    }
  } 
  else 
  {
    if(prev_disp_mask & SHOW_TEMP_ICON )
    {

       //  set correct rotation 
      tft.setRotation(1);
    
       tft.fillRect (x_i, y_i, temp_icon_width, temp_icon_height, BLACK);

       //Serial.println("Clear Icon!");
    }
  }

 // update text if necessary
    
  if (disp_mask & SHOW_TEMP_TEXT)
  {
    if(temp != prev_temp || !(prev_disp_mask & SHOW_TEMP_TEXT))
    {
      itoa(temp, buf, 10);
      y += printText(buf, x, y);
      y += printText("?C", x, y);

      //Serial.println("New Text!");
    }
  }
  else 
  {
    if(prev_disp_mask & SHOW_TEMP_TEXT)
    {
        uint8_t len;
        itoa(prev_temp, buf, 10);
        len = strlen(buf);
        // print padding
        for(int i = 0; i< 2*len + 6; i++)
          y +=  printText(" ", x, y);

        //Serial.println("Clear Text!");
    }
  }


  prev_temp = temp;
  prev_disp_mask = disp_mask;
  
}


//static uint8_t weather_icon_x = 7;
//static uint8_t weather_icon_y = 78; 

  
/*Definiton of weather widget */


#define REDRAW_WEATHER (1<<7)
#define SHOW_WEATHER_ICON (1<<1)
#define SHOW_WEATHER_TEXT (1<<0)


unsigned char const* _getPixleBuffer(uint8_t weather_icon_index)
{
  switch(weather_icon_index) 
  {
    case 0: // unknown
        return &image_data_weather_unknown[0];
    case 1:// sunny
        return &image_data_weather_sunny[0];
    case 2: //cloudy
        return &image_data_weather_cloudy[0];
    case 3: //partially cloudy
        return &image_data_weather_partly_cloudy[0];
    case 4: // rain
        return &image_data_weather_rain[0];
    case 5: // snow
        return &image_data_weather_snow[0];
  }

  return NULL;
}

void displayWeatherWidget(uint8_t x, uint8_t y, uint8_t icon_index, int8_t temp_min, int8_t temp_max,  uint8_t disp_mask)
{
  static int8_t text_offest_x = weather_icon_height - 17;
  static int8_t text_offset_y = weather_icon_width + 4;
  static uint8_t text_height = 15;
  
  static int8_t prev_temp_min = 0;
  static int8_t prev_temp_max = 0;
  static uint8_t prev_disp_mask = 0;
  static uint8_t prev_icon_index = 0;

  char  buf[10];
  
  // the icon has a non standard rotation, addapt coordinates
  uint8_t x_i, y_i;
  x_i = y;
  y_i = 128 - x -  weather_icon_height;

  // update text start position
  x = x + text_offest_x;
  y = y + text_offset_y;

  
  if (disp_mask & REDRAW_WEATHER)
  {
    //  set correct rotation 
    tft.setRotation(1);
    
    // display icon 
    display_RLE_4BITGRAY(_getPixleBuffer(icon_index), x_i , y_i, weather_icon_width, weather_icon_height);

    // display text 
    // clear line 
    uint8_t o;
    /*
    printText("          ", x, y);
    itoa(temp_max, buf, 10);
    uint8_t o = printText(buf, x, y);
    printText("?C", x, y + o);
    */
    
    printText("          ", x-text_height, y);
    itoa(temp_min, buf, 10);
    o = printText(buf, x - text_height, y);
    printText("?C", x -text_height, y + o);

    // store new data.
    prev_temp_min = temp_min;
    prev_temp_max = temp_max;
    prev_icon_index = icon_index;
    prev_disp_mask = disp_mask | SHOW_WEATHER_ICON | SHOW_WEATHER_TEXT;  


    ////Serial.println("Redraw!");
    return;   
  }


  if( disp_mask & SHOW_WEATHER_ICON)
  {
      // if the icon changed
      if(prev_icon_index != icon_index || !(prev_disp_mask & SHOW_WEATHER_ICON))
      {
        //  set correct rotation 
        tft.setRotation(1);
        
        // display icon 
         display_RLE_4BITGRAY(_getPixleBuffer(icon_index), x_i , y_i, weather_icon_width, weather_icon_height);
       
        //Serial.println("Icon change!");
      }
  } else {
    
    if (prev_disp_mask & SHOW_WEATHER_ICON)  
    {
      //  set correct rotation 
      tft.setRotation(1);  

      tft.fillRect (x_i, y_i, weather_icon_width, weather_icon_height, BLACK);

       //Serial.println("Blank icon!");
    }
    
  }



  if( disp_mask & SHOW_WEATHER_TEXT)
  {
     if(prev_temp_min != temp_min || prev_temp_max!= temp_max || !(prev_disp_mask & SHOW_TEMP_TEXT))
     {
      // display text 
      uint8_t o;
      /*
      itoa(temp_max, buf, 10);
      uint8_t o = printText(buf, x, y);
      printText("?C", x, y + o);
      */
      itoa(temp_min, buf, 10);
      o = printText(buf, x - text_height, y);
      printText("?C", x -text_height, y + o);
  
      //Serial.println("New Text!");
      }  
  } else {
    
    if (prev_disp_mask & SHOW_WEATHER_TEXT)
    {
      uint8_t len, tmp = y;
      /*
      itoa(prev_temp_max, buf, 10);
      len = strlen(buf);
      // print padding
      for(int i = 0; i< 2*len + 4; i++)
        y +=  printText(" ", x, y);
      */
      
      y = tmp;
      itoa(prev_temp_min, buf, 10);
      len = strlen(buf);
      // print padding
      for(int i = 0; i< 2*len + 4; i++)
        y +=  printText(" ", x-text_height, y);

      //Serial.println("Blank Text!");
    }
  
  }
    
    prev_temp_min = temp_min;
    prev_temp_max = temp_max;
    prev_icon_index = icon_index;

    prev_disp_mask = disp_mask;
}



/*Alarm widget definiton*/

//static uint8_t alarm_icon_x = 100;
///static uint8_t alarm_icon_y = 134;

#define REDRAW_ALARM (1<<7)
#define SHOW_ALARM_ICON (1<<0)

  
void displayAlarmWidget(uint8_t x, uint8_t y, uint8_t alarm_icon, uint8_t disp_mask)
{
  static uint8_t prev_alarm_icon;
  static uint8_t prev_disp_mask;

  if(disp_mask & REDRAW_ALARM)
  {
    // set origin at (0, 0)
    //  set correct rotation (font is rotated 90deg for better RLE compression)
    tft.setRotation(0);
    
    if (alarm_icon == 1)
      // render character 
      display_RLE_4BITGRAY(&image_data_alarm_set[0], x, y, alarm_icon_width, alarm_icon_height);
    else 
      display_RLE_4BITGRAY(&image_data_alarm_off[0], x, y, alarm_icon_width, alarm_icon_height);  

    prev_alarm_icon = alarm_icon;
    prev_disp_mask = disp_mask;

    return;
  }

  if (disp_mask & SHOW_ALARM_ICON)
  {
    if(alarm_icon != prev_alarm_icon || !(prev_disp_mask & SHOW_ALARM_ICON))
    {
    //  set correct rotation (font is rotated 90deg for better RLE compression)
    tft.setRotation(0);
    
    if (alarm_icon == 1)
      // render character 
      display_RLE_4BITGRAY(&image_data_alarm_set[0], x, y, alarm_icon_width, alarm_icon_height);
    else 
      display_RLE_4BITGRAY(&image_data_alarm_off[0], x, y, alarm_icon_width, alarm_icon_height);  

        //Serial.println("New icon!");
    }  


  } 
  else 
  {
    if(prev_disp_mask & SHOW_ALARM_ICON)
    {
      //  set correct rotation (font is rotated 90deg for better RLE compression)
      tft.setRotation(0);

      tft.fillRect(x, y, alarm_icon_width, alarm_icon_height, BLACK);

       //Serial.println("Clear icon!");
      
    }
  
  }

  prev_alarm_icon = alarm_icon;
  prev_disp_mask = disp_mask;
}


/*time widget definiton*/

//static uint8_t time_icon_x = 69;
///static uint8_t time_icon_y = 128;
  
#define REDRAW_TIME (1<<7)
#define SHOW_TIME_ICON (1<<0)
#define SHOW_TIME_FORMAT (1<<1)

#define AM_ICON  0
#define PM_ICON  1
#define BLANK_ICON  2

#define _24H_FORMAT_ICON 0
#define _12H_FORMAT_ICON 1
  
void displayTimeWidget(uint8_t x, uint8_t y, uint8_t time_icon, uint8_t format_icon ,uint8_t disp_mask)
{
  static uint8_t prev_time_icon;
  static uint8_t prev_format_icon;
  static uint8_t prev_disp_mask;

  if(disp_mask & REDRAW_TIME)
  {
    //  set correct rotation (font is rotated 90deg for better RLE compression)
    tft.setRotation(0);

    if(format_icon == _12H_FORMAT_ICON )
    {
      if (time_icon == AM_ICON) // AM
       display_RLE_4BITGRAY(&image_data_AM[0], x, y, time_icon_width, time_icon_height);
      else if(time_icon == PM_ICON) //PM
         display_RLE_4BITGRAY(&image_data_PM[0], x, y, time_icon_width, time_icon_height);
    }
    else // blank icon
      tft.fillRect(x, y, time_icon_width, time_icon_height, BLACK);
      
    prev_time_icon = time_icon;
    prev_format_icon = format_icon;
    prev_disp_mask = disp_mask;

    return;
  }

  if (disp_mask & SHOW_TIME_FORMAT)
  {
    if(format_icon != prev_format_icon|| !(prev_disp_mask & SHOW_TIME_FORMAT))
    {
      //  set correct rotation (font is rotated 90deg for better RLE compression)
      tft.setRotation(0);
      if (format_icon == _24H_FORMAT_ICON) // AM
       display_RLE_4BITGRAY(&image_data_24H[0], x + time_icon_width , y + 2, time_icon_width, time_icon_height);
      else if(format_icon == _12H_FORMAT_ICON) //PM
       display_RLE_4BITGRAY(&image_data_12H[0], x + time_icon_width, y + 2, time_icon_width, time_icon_height);
       
    }  
  } 
  else 
  {
    if(prev_disp_mask & SHOW_TIME_FORMAT)
    {
      //  set correct rotation (font is rotated 90deg for better RLE compression)
      tft.setRotation(0);

      tft.fillRect(x + time_icon_width, y, time_icon_width, time_icon_height, BLACK);

      //Serial.print("Clear icon");
    }
  
  }
  
   
  if (disp_mask & SHOW_TIME_ICON)
  {
    if(time_icon != prev_time_icon || !(prev_disp_mask & SHOW_TIME_ICON) || prev_format_icon != format_icon)
    {
    //  set correct rotation (font is rotated 90deg for better RLE compression)
    tft.setRotation(0);
    
    if(format_icon == _12H_FORMAT_ICON )
    {
      if (time_icon == AM_ICON) // AM
       display_RLE_4BITGRAY(&image_data_AM[0], x, y, time_icon_width, time_icon_height);
      else if(time_icon == PM_ICON) //PM
         display_RLE_4BITGRAY(&image_data_PM[0], x, y, time_icon_width, time_icon_height);
    }
    else // blank icon
      tft.fillRect(x, y, time_icon_width, time_icon_height, BLACK);
      
    }  
  } 
  else 
  {
    if(prev_disp_mask & SHOW_TIME_ICON)
    {
      //  set correct rotation (font is rotated 90deg for better RLE compression)
      tft.setRotation(0);

      tft.fillRect(x, y, time_icon_width, time_icon_height, BLACK);

      //Serial.print("Clear icon");
    }
  
  }
  
  prev_time_icon = time_icon;
  prev_format_icon = format_icon;
  prev_disp_mask = disp_mask;

}




#endif
