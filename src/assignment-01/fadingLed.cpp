#include "Arduino.h"

#define RED_LED 6                 //Ls
#define RED_LED_DELAY 20          //For fading LED


extern int currIntensity;
extern int fadeAmount;

void redLEDFading(){
  analogWrite(RED_LED, currIntensity);
  currIntensity = currIntensity + fadeAmount;
  if (currIntensity == 0 || currIntensity == 255) {
    fadeAmount = -fadeAmount ;
  }
  delay(RED_LED_DELAY);
}