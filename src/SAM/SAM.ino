/*
This sketch shows how to use other fonts
and some new text commands

 */

#include <TFT_ST7735.h>
#include <DS3231.h>

#include "widget.h"
#include "buttons.h"
//#include "time.h"
DS3231 rtclock;
RTCDateTime now;



#define __CS  10
#define __DC  9

TFT_ST7735 tft = TFT_ST7735();//we dont use rst, tie reset pin to 3v3

// widget positions

uint8_t clock_widget_x = 69;
uint8_t clock_widget_y = 7;

uint8_t weather_widget_x = 0;
uint8_t weather_widget_y = 7;

uint8_t temp_widget_x = 0;
uint8_t temp_widget_y = 95;

uint8_t date_widget_x = 50;
uint8_t date_widget_y = 7;

uint8_t time_widget_x = 69;
uint8_t time_widget_y = 128;

uint8_t alarm_widget_x = 102;
uint8_t alarm_widget_y = 136;

// state of alarm indicator
uint8_t alarm_on = 0;

// state of weather indicator
int8_t min_temp = -42;
int8_t max_temp = -42;
uint8_t weather_icon = 0; // 0 - 6 

// ambient temperature indicator
int8_t ambient_temp = -25;

// state of time indicator icon

uint8_t am_pm = 0;
uint8_t time_format = 0; // 0 - 24h.. 1-12h 


// alarm hour minut and second
uint8_t alarm_hour = 0; // stored in 24h format 
uint8_t alarm_minute = 0;
uint8_t alarm_second = 0;
uint8_t alarm_am_pm = 0;


// stop alarm
uint8_t alarm_set = 0;
uint8_t alarm_cleared = 0;


//Menu states 
#define MAIN_DISPLAY  0
// hover in settings menu 
// time related
#define HOVER_SEC 1 // entry point in settings
#define HOVER_MIN 2
#define HOVER_HOUR 3
#define HOVER_FORMAT 4 
#define HOVER_AM_PM 5
 
// date related 
#define HOVER_MONTH 6
#define HOVER_YEAR 7
#define HOVER_DAY 8

// select in settings menu
#define SELECT_SEC 9 // entry point in settings
#define SELECT_MIN 10
#define SELECT_HOUR 11
#define SELECT_FORMAT 12 
#define SELECT_AM_PM 13

// date related 
#define SELECT_MONTH 14
#define SELECT_YEAR 15
#define SELECT_DAY 16


// alarm 
#define HOVER_ALARM 17
#define HOVER_ALARM_AM_PM 18

#define HOVER_ALARM_SEC 19
#define HOVER_ALARM_MIN 20
#define HOVER_ALARM_HOUR 21


#define SELECT_ALARM_SEC 22
#define SELECT_ALARM_MIN 23 
#define SELECT_ALARM_HOUR 24 

#define SYNC_MENU 25


#define WIFI_CONNECTED ( (char)0x01)
#define TCP_CONNECTED (char)0x02
#define TCP_NOT_CONNECTED (char)0x03
#define HTTP_RESPONSE_OK (char)0x04
#define HTTP_RESPONSE_NOT_OK (char)0x05
#define JSON_OK (char)0x6
#define JSON_NOT_OK (char)0x7
#define UDP_CONNECTED (char)0x8
#define UDP_NOT_CONNECTED (char)0x9
#define UDP_RESPONSE_OK (char)0x10
#define UDP_RESPONSE_NOT_OK (char)0x11


#define GET_WEATHER_UPDATE 'w'
#define GET_TIME_UPDATE 't'

uint8_t menu_state = MAIN_DISPLAY;

static uint32_t prev_millis = 0;
static uint16_t toggle_millis = 500;
static uint8_t toggle_state = 0;


void _beep()
{
  tone(4, 3000, 50);
}


void _beep2()
{
  tone(4, 3000, 50);
  delay(200);
  tone(4, 3000, 50);
}
void _reset_toggle()
{
  toggle_state = 0;
  prev_millis = millis();
}

void _update_toggle()
{
  if(millis()- prev_millis > toggle_millis)
   {
      toggle_state ^= 0x1;
      prev_millis = millis();
   }
}

uint8_t hour12(uint8_t hour, uint8_t* am_pm)
{
  // convert to 12 hour mode
  if (hour == 0) {
    *am_pm = 0; // am
    return hour = 12; 
  }

  if(hour == 12) {
    *am_pm = 1; // pm
    return hour;
  }
    
  if(hour > 12) {
    *am_pm = 1; // pm 
    return hour - 12;  
  }
  
  // between 12 am and 12 pm 
  *am_pm = 0;
  return hour;
}

uint8_t hour24(uint8_t hour, uint8_t am_pm)
{
  if(am_pm == 0) // am
  {
    if(hour ==  12 )
      return 0;

    return hour;
  }
  else // pm 
  {
    if(hour == 12)
      return 12;

    return hour + 12;
  }
}

void saveModifications()
{
  // check the time format 
  if(time_format == 1)
  {
    // convert time to 24h
    now.hour = hour24( now.hour, am_pm);  
  }  

  // write data to rtc
  rtclock.setDateTime(now.year, now.month, now.day, now.hour, now.minute, now.second); 
}

void saveAlarmModifications()
{
  // store alarm on rtc 
  if(time_format == 1)
      rtclock.setAlarm1(0, hour24(alarm_hour, alarm_am_pm), alarm_minute, alarm_second , DS3231_MATCH_H_M_S);
    else 
      rtclock.setAlarm1(0, alarm_hour, alarm_minute, alarm_second , DS3231_MATCH_H_M_S);

}


void checkAlarm()
{
  static unsigned long alarm_start = 0;
  uint8_t toggle = 0;
  
  if(rtclock.isAlarm1() && alarm_on)
  {
    alarm_set = 1;  
    alarm_cleared = 0;
    alarm_start = millis();
  }
      
  if (alarm_set &&  !alarm_cleared && (( (unsigned long)millis() - alarm_start) > 60*1000))
  {
   // Serial.println(now_t, DEC);
  //  Serial.println(alarm_start, DEC);
    alarm_set = 0;
    alarm_cleared = 0;  
  }

  if( alarm_on && alarm_set && !alarm_cleared)
  {
      if(toggle == 0)
      {
        noTone(4);
        delay(50);

        toggle = 1;
      }
      if(toggle == 1)
      {
        tone(4, 3000, 50);
        delay(50);

        toggle = 0;
      }
  }
}


uint8_t waitResponse()
{
  unsigned long start = millis();
  
  while(!Serial.available() &&  (( (unsigned long) millis() - start) < 10000 ) ) {};
  return Serial.read();

  return 0xff; // no code 
}

unsigned long lastTempRead = 0;
void updatDisplay()
{
  
  switch(menu_state) 
  {
    case MAIN_DISPLAY:
    {
      // get new time interval
      now = rtclock.getDateTime();
      if( millis() - lastTempRead > 2000)
      {
        rtclock.forceConversion();
        ambient_temp = rtclock.readTemperature();
        lastTempRead = millis();
      }
      // check alarm
      checkAlarm();
      
      // if we are in 12 hour mode convert hour before display
      if(time_format == 1)
        now.hour = hour12(now.hour, &am_pm);
      
      // update display 
      displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      if(weather_icon == 0)
        displayWeatherWidget(weather_widget_x, weather_widget_y, weather_icon, max_temp, min_temp, SHOW_WEATHER_ICON);
      else 
         displayWeatherWidget(weather_widget_x, weather_widget_y, weather_icon, max_temp, min_temp, SHOW_WEATHER_ICON|SHOW_WEATHER_TEXT);
      displayTempWidget(temp_widget_x, temp_widget_y, ambient_temp, SHOW_TEMP_ICON|SHOW_TEMP_TEXT);         
      displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format,SHOW_TIME_ICON );
      displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, SHOW_ALARM_ICON);
      
      
      // check events 
      
      // minus and plus button pressed should be igonored            
      if ((select_button_pressed) && (select_button_hold_time > long_press_millis) )
      {
          // enter settings menu 
          menu_state = HOVER_FORMAT;
          clearSelectButton(); 

          // beep
          _beep();
      
          // reset toggle state 
          _reset_toggle();
      }

      if ((minus_button_pressed) && (minus_button_hold_time > long_press_millis) )
      {
          // enter settings menu 
          menu_state = HOVER_ALARM;
          clearMinusButton(); 

          // beep
          _beep();
      
          // reset toggle state 
          _reset_toggle();

          // when transiting to Alaram menu hide unnecessary data
          displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
          displayWeatherWidget(weather_widget_x, weather_widget_y, weather_icon, max_temp, min_temp, 0x0);
          displayTempWidget(temp_widget_x, temp_widget_y, ambient_temp, 0x0);         
          displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, 0x0);
          displayTimeWidget(time_widget_x, time_widget_y, alarm_am_pm, time_format, SHOW_TIME_ICON );
          displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, SHOW_ALARM_ICON);
      } 

      if ((plus_button_pressed) && (plus_button_hold_time > long_press_millis) )
      {
          // enter settings menu 
          menu_state = SYNC_MENU;
          clearPlusButton(); 

          // beep
          _beep();
      
          // reset toggle state 
          _reset_toggle();

          // when transiting to Alaram menu hide unnecessary data
          displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, 0x0);
          displayWeatherWidget(weather_widget_x, weather_widget_y, weather_icon, max_temp, min_temp, 0x0);
          displayTempWidget(temp_widget_x, temp_widget_y, ambient_temp, 0x0);         
          displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, 0x0);
          displayTimeWidget(time_widget_x, time_widget_y, alarm_am_pm, time_format, 0x0 );
          displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, 0x0);

          //
          //echo_x = 128-19;
          //echo_y = 0;
          tft.setRotation(0);
          tft.fillRect(0,0, 128, 160, TFT_BLACK);
      } 
      

      // clear alarm
      if (alarm_on && alarm_set && (select_button_released || minus_button_released ||plus_button_released ))
        alarm_cleared = 1;
        
      // clear shor press 
      if (select_button_released)
        clearSelectButton();  

      if (minus_button_released)
        clearMinusButton(); 
      
      if (plus_button_released)
        clearPlusButton();  
         
      break;
    }
    case HOVER_AM_PM:
    {  
      
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      else 
        displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT );
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_FORMAT;
          clearPlusButton(); 
          //_beep(); 
      
          // reset toggle state 
          _reset_toggle();
          displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      }else if (minus_button_pressed && minus_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_MONTH;
         clearMinusButton();  
         //_beep();
      
         // reset toggle state 
         _reset_toggle();
          displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      }else if (select_button_pressed && select_button_released )
      {
         // enter settings menu 
         // menu_state = SELECT_AM_PM;
         am_pm = !am_pm;
         
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
         toggle_state = 1;
      
         // display clock widget
         displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      } else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY;
        clearSelectButton(); 
        _beep2();

        // store to rtc
        saveModifications();  
  
      }
      break;
    }
  
    case HOVER_FORMAT:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      else 
        displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_ICON);
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {

          menu_state = HOVER_SEC;

          clearPlusButton(); 
          //_beep(); 
      
          // reset toggle state 
          _reset_toggle();
          displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      }else if (minus_button_pressed && minus_button_released)
      {
         // enter settings menu 
          if (time_format == 1)
             menu_state = HOVER_AM_PM;
          else 
             menu_state = HOVER_MONTH;
             
         clearMinusButton();  
         //_beep();
      
         // reset toggle state 
         _reset_toggle();
         displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      }else if (select_button_pressed && select_button_released && (select_button_hold_time >= debounce_time_millis) )
      {
         // enter settings menu 
         //menu_state = SELECT_FORMAT;

         if (time_format == 0) // 24h
          {
            time_format = 1; // switch to 12h

            // convert to 12 hour mode
            now.hour = hour12(now.hour, &am_pm);

            // convert alarm to 12 hour mode 
            alarm_hour = hour12(alarm_hour, &alarm_am_pm);
          } else 
          {
            time_format = 0; // switch to 24h 
            now.hour = hour24(now.hour, am_pm);

            alarm_hour = hour24(alarm_hour, alarm_am_pm);
          }

          // show new time
          displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
          
        
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
         toggle_state = 1;
      
         // display clock widget
         displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, SHOW_TIME_FORMAT | SHOW_TIME_ICON);
      }else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY;
        clearSelectButton();  
        _beep2();
        // store to rtc
        saveModifications();  
      
        // display clock widget
      }

      if(select_button_released)
        clearSelectButton();
      break;
    }
  
    case HOVER_SEC:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN);
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_MIN;
          clearPlusButton(); 
          //_beep(); 
      
          // reset toggle state 
          _reset_toggle();
      } 
      else if (minus_button_pressed && minus_button_released)
      {
        
         // enter settings menu 
         menu_state = HOVER_FORMAT;
         clearMinusButton();  
         //_beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_SEC;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY;
        clearSelectButton(); 
        _beep2();
        // store to rtc
        saveModifications();  
      
        // display clock widget
      }
      break;
    }
    case SELECT_SEC:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state == 0)
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN);
      
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          now.second = (now.second+1)%60;
          clearPlusButton();  
      
          // reset toggle state 
          _reset_toggle();
      
      }else if (minus_button_pressed && minus_button_released)
      {
         // enter settings menu 
         now.second = ((now.second == 0)? 59 : now.second - 1)%60;
         clearMinusButton();  
      
         // reset toggle state 
         _reset_toggle();
      
      
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_SEC;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         //displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }
      break;   
  
    }
    case HOVER_MIN:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_SEC);
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_HOUR;
          clearPlusButton();  
          //_beep();
      
          // reset toggle state 
          _reset_toggle();
      } else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_SEC;
          clearMinusButton();  
          //_beep();
      
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_MIN;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY;
        clearSelectButton(); 
        _beep2();

        // store to rtc
        saveModifications();  
      
        // display clock widget
      }
      break;
    }
    case SELECT_MIN:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state == 0)
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_SEC);
    
       // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          now.minute = (now.minute+1)%60;
          clearPlusButton();  
    
          // reset toggle state 
          _reset_toggle();
    
      }else if (minus_button_pressed && minus_button_released)
      {
         // enter settings menu 
         now.minute = ((now.minute == 0)? 59 : now.minute - 1)%60;
         clearMinusButton();  
    
         // reset toggle state 
         _reset_toggle();
    
    
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_MIN;
         clearSelectButton();  
         _beep();
    
         // reset toggle state 
         _reset_toggle();
    
         // display clock widget
        // displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }
      break;   
    }
    case HOVER_HOUR:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_MIN|SHOW_SEC);
    
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_YEAR;
          clearPlusButton();  
          //_beep();
    
          // reset toggle state 
          _reset_toggle();
    
          // display latest values 
          displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_MIN;
          clearMinusButton(); 
          //_beep(); 
          
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_HOUR;
         clearSelectButton();
         _beep();  
    
         // reset toggle state 
         _reset_toggle();
    
         // display clock widget
         displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY; 
        clearSelectButton(); 
        _beep2();

        // store to rtc
        saveModifications();  
      
        // display clock widget
      }
      break;
    }
    case SELECT_HOUR:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state == 0)
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_MIN|SHOW_SEC);
    
    
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          if(time_format == 0) // 24_hour mode
          {
            now.hour = (now.hour+1)%24;
          } 
          else 
          if(time_format == 1)
          {
            now.hour = (now.hour+1) % 13;
            now.hour = (now.hour == 0 )? 1: now.hour;
          }
          clearPlusButton();  
    
          // reset toggle state 
          _reset_toggle();
    
      }else if (minus_button_pressed && minus_button_released)
      {
          if(time_format == 0) // 24_hour mode
          {
            now.hour = ((now.hour == 0)? 23 : now.hour - 1)%24;
          } 
          else 
          if(time_format == 1)
          {
            now.hour = ((now.hour == 1)? 12 : now.hour - 1)%13;
          }
          
         // enter settings menu 
         clearMinusButton();  
    
         // reset toggle state 
         _reset_toggle();
    
    
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_HOUR;
         clearSelectButton();  
        _beep();
         // reset toggle state 
         _reset_toggle();
    
         // display clock widget
         //displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }
      break;   
    }
    case HOVER_YEAR:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      // update only affected areas 
      if(toggle_state)
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      else 
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH);
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_DAY;
          clearPlusButton();  
          //_beep();
      
          // reset toggle state 
          _reset_toggle();
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_HOUR;
          clearMinusButton(); 
          //_beep(); 
          
          // reset toggle state 
          _reset_toggle();
      
          // display latest data 
          displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_YEAR;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      }else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY; 
        clearSelectButton(); 
        _beep2();

        // store to rtc
        saveModifications();  
      
        // display clock widget
      }
      break;
    }   
    case SELECT_YEAR:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      // update only affected areas 
      if(toggle_state == 0)
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      else 
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH);
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          now.year = now.year + 1;
          
          // update day of week 
          now.dayOfWeek = rtclock.dow(now.year, now.month, now.day);
          
          clearPlusButton();  
      
          // reset toggle state 
          _reset_toggle();
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          now.year = (now.year == 0) ? 0 : now.year - 1 ;
          
          // update day of week 
          now.dayOfWeek = rtclock.dow(now.year, now.month, now.day);
          
          clearMinusButton();  
          
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_YEAR;
         clearSelectButton(); 
         _beep(); 
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         //displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      }
      break;        
    }
    case HOVER_DAY:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      // update only affected areas 
      if(toggle_state)
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      else 
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_MONTH|SHOW_YEAR);
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_MONTH;
          clearPlusButton();  
          //_beep();
      
          // reset toggle state 
          _reset_toggle();
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_YEAR;
          clearMinusButton();  
          //_beep();
          
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_DAY;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      }else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY; 
        clearSelectButton(); 
        _beep2();

        // store to rtc
        saveModifications();  
      
        // display clock widget
      }
      break;
    }
    case SELECT_DAY:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      // update only affected areas 
      if(toggle_state == 0)
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      else 
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_MONTH|SHOW_YEAR);
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          uint8_t nr_days = rtclock.daysInMonth(now.year, now.month);
          
          // enter settings menu 
          now.day = now.day + 1;
          if (now.day > nr_days)
            now.day = 1;
    
          // update day of week 
          now.dayOfWeek = rtclock.dow(now.year, now.month, now.day);
          
          clearPlusButton();  
    
          // reset toggle state 
          _reset_toggle();
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          uint8_t nr_days = rtclock.daysInMonth(now.year, now.month);
          
          // enter settings menu 
          now.day = now.day - 1;
          if (now.day < 1)
            now.day = nr_days;
    
          // update day of week 
          now.dayOfWeek = rtclock.dow(now.year, now.month, now.day);
            
          clearMinusButton();  
          
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_DAY;
         clearSelectButton();  
         _beep();
         // reset toggle state 
         _reset_toggle();
    
         // display clock widget
         //displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      }
      break;   
    }
    case HOVER_MONTH:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      // update only affected areas 
      if(toggle_state)
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      else 
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_YEAR);
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          if(time_format == 1 )
            menu_state = HOVER_AM_PM;
          else
            menu_state = HOVER_FORMAT;
          clearPlusButton();  
          //_beep();
          
          // reset toggle state 
          _reset_toggle();
    
          // draw last state 
          displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_DAY;
          clearMinusButton();  
          //_beep();
          
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_MONTH;
         clearSelectButton();  
        _beep();
        
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      }else if(select_button_pressed && (select_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY;
        clearSelectButton(); 
        _beep2();
        
        // store to rtc
        saveModifications();  
      
        // display clock widget
      }
      break;
    }
    case SELECT_MONTH:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      // update only affected areas 
      if(toggle_state == 0)
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      else 
        displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_YEAR);
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          now.month = now.month + 1;
          if(now.month > 12)
            now.month = 1;
          
          // clear event 
          clearPlusButton();  
          
          // validate day
          uint8_t nr_days = rtclock.daysInMonth(now.year, now.month);
          if (now.day > nr_days)
            now.day = 1;
            
          clearPlusButton();  

           // update day of week 
          now.dayOfWeek = rtclock.dow(now.year, now.month, now.day);
          
          // reset toggle state 
          _reset_toggle();
    
          // draw last state 
          displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          
          if(now.month == 1)
            now.month = 12;
           else 
            now.month = now.month - 1;
          
          // clear event 
          clearMinusButton();  
          
          // validate day
          uint8_t nr_days = rtclock.daysInMonth(now.year, now.month);

          if (now.day > nr_days)
            now.day = 1;

            
           // update day of week 
          now.dayOfWeek = rtclock.dow(now.year, now.month, now.day);
          
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_MONTH;
         clearSelectButton();  

         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
        // displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, SHOW_DAY|SHOW_MONTH|SHOW_YEAR);
      }
      break;
    
    }

// alarm shit(stuff)


    case HOVER_ALARM:
    {
     // toggle sec every 500 milliseconds 
    _update_toggle();
    // update only affected areas 
    if(toggle_state )
        displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, SHOW_ALARM_ICON);
    else 
       displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, 0x0);
    
    // check events 
    if (plus_button_pressed && plus_button_released)
    {
        // enter settings menu 
        menu_state = HOVER_ALARM_SEC;
        clearPlusButton();  
        //_beep();
    
        // reset toggle state 
        _reset_toggle();
        
        // draw last state 
        displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, SHOW_ALARM_ICON);
    } 
    else if(minus_button_pressed && minus_button_released && (minus_button_hold_time > debounce_time_millis))
    {

        if(time_format == 1)
          menu_state = HOVER_ALARM_AM_PM;
        else 
          menu_state = HOVER_ALARM_HOUR; // HOUR
        
        clearMinusButton();  
        //_beep();
        
        // reset toggle state 
        _reset_toggle();
    
        // draw last state 
        displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, SHOW_ALARM_ICON);
    }else if (select_button_pressed && select_button_released)
    {
       // enter settings menu 
       alarm_on = !alarm_on;
       clearSelectButton();  
        
       _beep();

  
       // reset toggle state 
       _reset_toggle();
       toggle_state = 1;

    }else if(minus_button_pressed && (minus_button_hold_time > long_press_millis) )
    {
      // switch back to main menu
      menu_state = MAIN_DISPLAY;
      clearMinusButton(); 
      _beep2();
   
      
      // save  
      saveAlarmModifications();
    }

    if(minus_button_released)
      clearMinusButton();
    
    break;
    }      
    case HOVER_ALARM_AM_PM:
    {  
      
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayTimeWidget(time_widget_x, time_widget_y, alarm_am_pm, time_format,  SHOW_TIME_ICON);
      else 
        displayTimeWidget(time_widget_x, time_widget_y, alarm_am_pm, time_format, 0x0 );
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_ALARM;
          clearPlusButton(); 
          //_beep(); 
      
          // reset toggle state 
          _reset_toggle();
          displayTimeWidget(time_widget_x, time_widget_y, alarm_am_pm, time_format, SHOW_TIME_ICON);
      }else if (minus_button_pressed && minus_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_ALARM_SEC; // HOUR
         clearMinusButton();  
         //_beep();
      
         // reset toggle state 
         _reset_toggle();
          displayTimeWidget(time_widget_x, time_widget_y, alarm_am_pm, time_format, SHOW_TIME_ICON);
      }else if (select_button_pressed && select_button_released )
      {
         // enter settings menu 
         // menu_state = SELECT_AM_PM;
         alarm_am_pm = !alarm_am_pm;
         
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
         toggle_state = 1;
      
         // display clock widget
         displayTimeWidget(time_widget_x, time_widget_y, alarm_am_pm, time_format, SHOW_TIME_ICON);
      } else if(minus_button_pressed && (minus_button_hold_time > long_press_millis) )
     {
      // switch back to main menu
      menu_state = MAIN_DISPLAY;
      clearMinusButton(); 
      _beep2();
      
       saveAlarmModifications();
      }
      break;
    }
    case HOVER_ALARM_SEC:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN);
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_ALARM_MIN;
          clearPlusButton(); 
          //_beep(); 
      
          // reset toggle state 
          _reset_toggle();
      } 
      else if (minus_button_pressed && minus_button_released)
      {
        
         // enter settings menu 
          menu_state = HOVER_ALARM;
         clearMinusButton();  
         //_beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_ALARM_SEC;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }else if(minus_button_pressed && (minus_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY;
        clearMinusButton(); 
        _beep2();
        
        saveAlarmModifications();
      }
      break;
    }
    case SELECT_ALARM_SEC:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state == 0)
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN);
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          alarm_second = (alarm_second+1)%60;
          clearPlusButton();  
      
          // reset toggle state 
          _reset_toggle();
      
      }else if (minus_button_pressed && minus_button_released)
      {
         // enter settings menu 
         alarm_second = ((alarm_second == 0)? 59 :alarm_second - 1)%60;
         clearMinusButton();  
      
         // reset toggle state 
         _reset_toggle();
      
      
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_ALARM_SEC;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         //displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }
      break;   
  
    }  
    case HOVER_ALARM_MIN:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_SEC);
      
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_ALARM_HOUR;
          clearPlusButton();  
          //_beep();
      
          // reset toggle state 
          _reset_toggle();
      } else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_ALARM_SEC;
          clearMinusButton();  
          //_beep();
      
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_ALARM_MIN;
         clearSelectButton();  
         _beep();
      
         // reset toggle state 
         _reset_toggle();
      
         // display clock widget
         displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }else if(minus_button_pressed && (minus_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY;
        clearMinusButton(); 
        _beep2();
   
         saveAlarmModifications();
      }
      break;
    }
    case SELECT_ALARM_MIN:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state == 0)
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_SEC);
      
    
       // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          alarm_minute = (alarm_minute+1)%60;
          clearPlusButton();  
    
          // reset toggle state 
          _reset_toggle();
    
      }else if (minus_button_pressed && minus_button_released)
      {
         // enter settings menu 
         alarm_minute = ((alarm_minute == 0)? 59 : alarm_minute - 1)%60;
         clearMinusButton();  
    
         // reset toggle state 
         _reset_toggle();
    
    
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_ALARM_MIN;
         clearSelectButton();  
         _beep();
    
         // reset toggle state 
         _reset_toggle();
      }
      break;   
    }
    case HOVER_ALARM_HOUR:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state)
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_MIN|SHOW_SEC);
      
    
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          if(time_format == 1)
            menu_state = HOVER_ALARM_AM_PM;
          else 
            menu_state = HOVER_ALARM;
            
          clearPlusButton();  
          //_beep();
    
          // reset toggle state 
          _reset_toggle();
    
          // display latest values 
           displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      } 
      else if(minus_button_pressed && minus_button_released)
      {
          // enter settings menu 
          menu_state = HOVER_ALARM_MIN;
          clearMinusButton(); 
          //_beep(); 
          
          // reset toggle state 
          _reset_toggle();
      }else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = SELECT_ALARM_HOUR;
         clearSelectButton();
         _beep();  
    
         // reset toggle state 
         _reset_toggle();
    
         // display latest values 
         displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
           
      }else if(minus_button_pressed && (minus_button_hold_time > long_press_millis) )
      {
        // switch back to main menu
        menu_state = MAIN_DISPLAY; 
        clearMinusButton(); 
        _beep2();

         saveAlarmModifications();
      }
      break;
    }
    case SELECT_ALARM_HOUR:
    {
      // toggle sec every 500 milliseconds 
      _update_toggle();
      
      // update only affected areas 
      if(toggle_state == 0)
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      else 
        displayClockWidget(clock_widget_x, clock_widget_y, alarm_hour,alarm_minute,alarm_second, SHOW_MIN|SHOW_SEC);
      
    
    
      
      // check events 
      if (plus_button_pressed && plus_button_released)
      {
          // enter settings menu 
          if(time_format == 0) // 24_hour mode
          {
            alarm_hour = (alarm_hour+1)%24;
          } 
          else 
          if(time_format == 1)
          {
            alarm_hour = (alarm_hour+1) % 13;
            alarm_hour = (alarm_hour == 0 )? 1: alarm_hour;
          }
          clearPlusButton();  
    
          // reset toggle state 
          _reset_toggle();
    
      }else if (minus_button_pressed && minus_button_released)
      {
          if(time_format == 0) // 24_hour mode
          {
            alarm_hour = ((alarm_hour == 0)? 23 : alarm_hour - 1)%24;
          } 
          else 
          if(time_format == 1)
          {
            alarm_hour = ((alarm_hour == 1)? 12 : alarm_hour - 1)%13;
          }
          
         // enter settings menu 
         clearMinusButton();  
    
         // reset toggle state 
         _reset_toggle();
    
    
      } else if (select_button_pressed && select_button_released)
      {
         // enter settings menu 
         menu_state = HOVER_ALARM_HOUR;
         clearSelectButton();  
        _beep();
         // reset toggle state 
         _reset_toggle();
    
         // display clock widget
         //displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, SHOW_HOUR|SHOW_MIN|SHOW_SEC);
      }
      break;   
    }

    case SYNC_MENU:
    {
      char data[30];
      uint8_t resp;
      uint8_t x = 128-19;
      uint8_t y = 0;
      uint8_t len = 0;
      // clear pending bytes
     while(Serial.available()) {
        resp = Serial.read();
     }    

      // update weather data
       printText("Update weather data",x, y);
       x -= 19;
      
      // send weather sync command 
      Serial.write(GET_WEATHER_UPDATE);
      resp = waitResponse();
      
      if(resp == WIFI_CONNECTED) {
        printText("WiFi connected",x, y);
        x -= 19;
        
        // wait for tcp connection status
        resp = waitResponse();
        if (resp == TCP_CONNECTED)
        {
          printText("Tcp connected",x, y);
          x -= 19;

          // wait for http response 
          resp = waitResponse();
          if(resp  == HTTP_RESPONSE_OK)
          {
            printText("Http response ok",x, y);
            x -= 19;

            // wait for json ok flag
            resp = waitResponse();
            if(resp == JSON_OK)
            {
              printText("JSON ok",x, y);
              x -= 19;

              while(Serial.available())
                data[len++] = Serial.read();
              data[len] = '\0';

              uint8_t i;
              // parse data
              max_temp = 0;
              for(i = 1; i<len-1; i++)
                if(data[i] != ',')
                  max_temp = max_temp*10 + (data[i]-'0');
                else 
                  break;

              //get weather icon 
              weather_icon = data[i+1] - '0';
            }else {
              printText("JSON not ok", x, y);
            }
            
          }else {
             printText("Http response not ok",x, y);
          }
          
        }else {
          printText("Not connected",x, y);
        }
      
      }else 
        printText("WiFi not connected", x, y);


    // clear screen
    tft.setRotation(0);
    tft.fillRect(0, 0, 128, 160, TFT_BLACK);
    
    x = 128-19;
    y = 0;
    len = 0;
    
      // Update time
     printText("Update time data",x, y);
     x-=19;
     Serial.write(GET_TIME_UPDATE);
      resp = waitResponse();
      
      if(resp == WIFI_CONNECTED) {
        printText("WiFi connected",x, y);
        x -= 19;
        
        // wait for tcp connection status
        resp = waitResponse();
        if (resp == UDP_CONNECTED)
        {
          printText("UDP connected",x, y);
          x -= 19;

          // check UDP response 
          resp = waitResponse();
          if(resp == UDP_RESPONSE_OK)
          {
            printText("UDP response ok",x,y);


            while(Serial.available())
              data[len++] = Serial.read();

            uint8_t i;
            // parse data
            uint8_t year = 0;
            for(i = 1; i<len-1; i++)
            if(data[i] != ',')
                year  = year*10 + (data[i]-'0');
            else 
              break;
              
            uint8_t month = 0;
            for(i = i+1; i<len-1; i++)
            if(data[i] != ',')
                month  = month*10 + (data[i]-'0');
            else 
              break;

            uint8_t day = 0;
            for(i = i+1; i<len-1; i++)
            if(data[i] != ',')
                day  = day*10 + (data[i]-'0');
            else 
              break;
                
            uint8_t hour = 0;
            for(i = i+1; i<len-1; i++)
            if(data[i] != ',')
                hour  = hour * 10 + (data[i]-'0');
            else 
              break;

            uint8_t minute = 0;
            for(i = i+1; i<len-1; i++)
            if(data[i] != ',')
                minute  = minute * 10 + (data[i]-'0');
            else 
              break;

            uint8_t second = 0;
            for(i = i+1; i<len-1; i++)
                second  = second*10 + (data[i]-'0');


            rtclock.setDateTime(year+1900, month + 1, day, hour, minute, second); 
        
          }else {
           printText("UDP response not ok",x, y);
          }
  
        }else {
          printText("UDP not connected",x, y);
        }
      
      }else 
        printText("WiFi not connected", x, y);
    
    
      while(menu_state == SYNC_MENU)
      {
        updateButtonStates();
        //echoInput();

  
        // clear any short press
        if (select_button_released)
          clearSelectButton();  
  
        if (minus_button_released)
          clearMinusButton(); 
        
        if (plus_button_released)
          clearPlusButton();  
  
  
        if ((plus_button_pressed) && (plus_button_hold_time > long_press_millis) )
        {
            // enter settings menu 
            menu_state = MAIN_DISPLAY;
            clearPlusButton(); 
  
            // beep
            _beep();
        
            // reset toggle state 
            _reset_toggle();

            // Clear background
            tft.setRotation(0);
            tft.fillRect(0, 0, 128, 160, TFT_BLACK);

            // redraw all
            displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, REDRAW_CLOCK);
            displayWeatherWidget(weather_widget_x, weather_widget_y, weather_icon, max_temp, min_temp, REDRAW_WEATHER);
            displayTempWidget(temp_widget_x, temp_widget_y, ambient_temp, REDRAW_TEMP);         
            displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, REDRAW_DATE);
            displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, REDRAW_TIME);
            displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, REDRAW_ALARM);
            
        } 
      }
    }
    
   }  

   

}


void resetLCD()
{
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  delay(1);
  digitalWrite(8, HIGH);
}



void setup() {
 
  Serial.begin(115200);
 // Serial.println("\nstarted\n");

  // init LCD
  resetLCD();
  tft.init();

  // init RTC
  rtclock.begin();
  // clear alarms 
  rtclock.armAlarm1(false);
  rtclock.clearAlarm1();
  
  
  // Set sketch compiling time
  //rtclock.setDateTime(__DATE__, __TIME__);

   // clear scren 
  tft.fillScreen(BLACK);
  
  // setup fonts
  initFont();
  initClockFont();

  // configure pins
  configurePins();

  now = rtclock.getDateTime();
  ambient_temp = rtclock.readTemperature();
  
  //displayCharacter('t', 50, 50);
  
  displayClockWidget(clock_widget_x, clock_widget_y, now.hour,now.minute,now.second, REDRAW_CLOCK);
  displayWeatherWidget(weather_widget_x, weather_widget_y, weather_icon, max_temp, min_temp, REDRAW_WEATHER);
  displayTempWidget(temp_widget_x, temp_widget_y, ambient_temp, REDRAW_TEMP);         
  displayDateWidget(date_widget_x, date_widget_y , now.year,now.month, now.day, now.dayOfWeek, REDRAW_DATE);
  displayTimeWidget(time_widget_x, time_widget_y, am_pm, time_format, REDRAW_TIME);
  displayAlarmWidget(alarm_widget_x, alarm_widget_y, alarm_on, REDRAW_ALARM);


  // clear serial buffer 
  while(Serial.available())
    Serial.read();
    
  _beep();
}



  
void loop() {
  delay(10);
  
  updateButtonStates();
  updatDisplay();

}
