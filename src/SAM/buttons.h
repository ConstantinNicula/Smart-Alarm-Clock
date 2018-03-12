#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#define minus_button_pin 7
#define select_button_pin 6
#define plus_button_pin 5


#define long_press_millis 1000

volatile uint8_t minus_button_pressed = 0; // the state of the button set by isr (cleared by software) 
uint8_t minus_button_released = 0;
volatile uint32_t minus_button_press_time = 0; // time in millis of button press
uint16_t minus_button_hold_time = 0;

volatile uint8_t select_button_pressed = 0; //  the state of the button set by isr
uint8_t select_button_released = 0;
volatile uint32_t select_button_press_time = 0;  // time in millis of button press
uint16_t select_button_hold_time = 0;

volatile uint8_t plus_button_pressed = 0; //  the state of the button set by isr
uint8_t plus_button_released = 0;
volatile uint32_t plus_button_press_time = 0;  // time in millis of button press
uint16_t plus_button_hold_time = 0;

#define debounce_time_millis 100


void clearMinusButton()
{
  minus_button_pressed = 0;
  minus_button_released = 0;
  minus_button_hold_time = 0;  
}

void clearSelectButton()
{
  select_button_pressed = 0;
  select_button_released = 0;
  select_button_hold_time = 0;  
}

void clearPlusButton()
{
  plus_button_pressed = 0;
  plus_button_released = 0;
  plus_button_hold_time = 0;  
}


void updateButtonStates()
{
    uint32_t t = millis();
    uint8_t pin_state;
    
    // minus button 
    if(minus_button_pressed && ((t - minus_button_press_time) > debounce_time_millis))
    {
      pin_state = !digitalRead(minus_button_pin);
      if(pin_state != 0 && !minus_button_released) // update hold time
        minus_button_hold_time = t - minus_button_press_time;
      else
         minus_button_released = 1;
    }


    // select button
    if(select_button_pressed && ((t - select_button_press_time) > debounce_time_millis))
    {
      pin_state = !digitalRead(select_button_pin);
      if(pin_state != 0 && !select_button_released) // update hold time
        select_button_hold_time = t - select_button_press_time;
      else
        select_button_released = 1;
    }


    // plus button 
    if(plus_button_pressed && ((t - plus_button_press_time) > debounce_time_millis))
    {
      pin_state = !digitalRead(plus_button_pin);
      if(pin_state != 0 && !plus_button_released) // update hold time
        plus_button_hold_time = t - plus_button_press_time;
      else 
        plus_button_released = 1;
    }
}

void pciSetup(byte pin)
{
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

ISR (PCINT2_vect) // handle pin change interrupt for D0 to D7 here
{
    uint8_t pin_state = !digitalRead(minus_button_pin);
    if (pin_state == 1 && !minus_button_pressed)
    {
        minus_button_pressed = 1;
        minus_button_press_time = millis();
    }


    pin_state = !digitalRead(select_button_pin);
    if (pin_state == 1 && !select_button_pressed)
    {
        select_button_pressed = 1;
        select_button_press_time = millis();
    }


   pin_state = !digitalRead(plus_button_pin);
   if (pin_state == 1 && !plus_button_pressed)
   {
      plus_button_pressed = 1;
      plus_button_press_time = millis();
   }

}  

void configurePins()
{

  pinMode(minus_button_pin, INPUT);
  pinMode(select_button_pin, INPUT);
  pinMode(plus_button_pin, INPUT);

  pciSetup(7);
  pciSetup(6);
  pciSetup(5); 
}
#endif
