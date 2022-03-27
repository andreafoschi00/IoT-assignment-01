#include "Arduino.h"
#include "fadingLed.h"


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