#include <avr/sleep.h>
#include <TimerOne.h>
#include <EnableInterrupt.h>
#include "fadingLed.h"

//WARNING: pin 9 and 10 were skipped because they are used by the TimerOne.h library
#define RED_LED 6                     //Ls
#define POT A0                        //Pot
#define NUM_LEDS 4                    //Number of leds (which is also the number of buttons)
#define GREEN_LED_DELAY 300           //Starting delay (decreased any time by 10%)
#define RED_LED_DELAY 20              //For fading LED
#define STARTING_TIME_IN_MILLIS 10000 //Game starts at 10sec response time (it will be decreased by the difficult set)
#define COMPLEXITY_FACTOR 0.1         //Factor F (0.1 --> level 1 | 0.8 --> level 8)

int ballSpeed;                        //Ball speed (ms)
int loopState;                        //Main variabile for switch constructor
int difficult;                        //To be setted by the potentiometer

int ledPin[] = {8, 11, 12, 13};       //L1, L2, L3, L4
int buttonPin[] = {2,3,4,5};          //T1, T2, T3, T4

unsigned long actualTime;             //For timer purposes
unsigned long movingTime;             //T1 --> Random from 3 to 5 seconds (in microSeconds)
unsigned long currentTime;            //T2 --> Starting from 10sec (at difficulty 1)

volatile int score;                   //Score of current game
volatile int currentPosition;         //The current position to guess

//Variables used for Ls
int brightness;
int fadeAmount;
int currIntensity;

bool newValueCatched;        //When goes true, the ball stopped to a position (L1..L4)
bool guessPhaseFinished;     //When goes true, the player guess correct or the game is over
bool sleeping;               //When goes true, the system is in deep-sleep mode
bool newGameRequested;       //When goes true, the player requested a new game (by pressing T1)

//That's just the code seen in lab
/*void redLEDFading(){
  analogWrite(RED_LED, currIntensity);
  currIntensity = currIntensity + fadeAmount;
  if (currIntensity == 0 || currIntensity == 255) {
    fadeAmount = -fadeAmount ;
  }
  delay(RED_LED_DELAY);
}*/

void initialState(){
  Serial.println("Welcome to the Catch the Bouncing Led Ball Game. Press Key T1 to Start");
  enableInterrupt(buttonPin[0], newGameStarting, RISING); //For launching a new game
  actualTime = millis();  //Start counting
  loopState = 1;
}

void waitForNextGame(){
  //Waiting 10 seconds
  if(millis() - actualTime < 10000) {
    if(newGameRequested) {
      return;
    }
    redLEDFading();
  } else {
    analogWrite(RED_LED, 0);
    disableInterrupt(buttonPin[0]);
    loopState = 2;
  }
}

void wakeUp(){
  sleep_disable();
  for(int x=0; x<NUM_LEDS; x++) {
    disableInterrupt(buttonPin[x]);
  }
  loopState = 0;
}

void sleepNow(){
  //Deep sleep (STEP #9 in lab)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  for(int x=0; x<NUM_LEDS; x++) {
    enableInterrupt(buttonPin[x], wakeUp, RISING);
  }
  Serial.println("Going into power save mode in 1s (Press any key to wake up).");
  Serial.flush();
  delay(1000);
  sleep_cpu();
}

void newGameStarting(){
  disableInterrupt(buttonPin[0]);
  newGameRequested = true;
  loopState = 3;
}

void newGame(){
  //Resetting board and variables before starting with the loop
  turnOffAllLEDs();
  newGameRequested = false;
  Serial.println("Go!");
  currentTime = STARTING_TIME_IN_MILLIS;
  ballSpeed = GREEN_LED_DELAY;
  score = 0;
  //WARNING: potentiometer has an unexpected behaviour: if you put on max difficult (8), it usually reads 7 (but it works fine in the waitForNextGame() method)
  difficult = map(analogRead(POT),0,1023,1,8);
  Serial.println("You set the difficult to: " + String(difficult));
  currentTime *= 1.1-(COMPLEXITY_FACTOR * difficult); //Difficult 1 -> 10sec to first try | Difficult 8 -> 3sec to first try
  delay(1000);
  loopState = 4;
}

void turnOffAllLEDs(){
  for(int x=0; x<NUM_LEDS; x++){
    digitalWrite(ledPin[x], LOW);
  }
  digitalWrite(RED_LED, LOW);
}

long newRandomTime(long vStart, long vEnd) {
  return random(vStart, vEnd);
}

void gameLoop() {
  turnOffAllLEDs();
  movingTime = newRandomTime(1000000, 5000000); // 1sec - 5sec
  Serial.println("Ball speed: " + String(ballSpeed) + " msec");
  Serial.println("Time to guess: " + String(currentTime) + " msec");
  newValueCatched = false;
  boolean ascending = true;
  int i=-1;
  Timer1.setPeriod(movingTime);
  Timer1.attachInterrupt(catchValue);
  //TODO: refactor in a new state!
  while(!newValueCatched) {
    if(i!=-1){
      digitalWrite(ledPin[i], LOW);
    }
    if(ascending) {
      if(i == NUM_LEDS-1) {
        ascending = false;
        i--;
      } else {
        i++;
      }
    } else {
      if(i == 0) {
        ascending = true;
        i++;
      } else {
        i--;
      }
    }
    digitalWrite(ledPin[i], HIGH);
    delay(ballSpeed);
  }
  Timer1.detachInterrupt();
  loopState = 5;
}

void catchValue() {
  //Saving the value where the ball stopped
  for(int x=0; x<NUM_LEDS-1; x++){
    if(digitalRead(ledPin[x]) == HIGH){
      noInterrupts();
      newValueCatched = true;
      currentPosition = x;
      interrupts();
    }
  }
}

void makeGuess() {
  //Attach guess interrupt to the buttons
  enableInterrupt(buttonPin[0], t1Pressed, RISING);
  enableInterrupt(buttonPin[1], t2Pressed, RISING);
  enableInterrupt(buttonPin[2], t3Pressed, RISING);
  enableInterrupt(buttonPin[3], t4Pressed, RISING);
  guessPhaseFinished = false;

  Timer1.setPeriod(currentTime * 1000); //millisec to microsec
  Timer1.attachInterrupt(timerEnded);
  loopState = 6;
}

void waitForGuess(){
  //Nothing happens, just waiting for interrupts
}

//TODO: is it possible to do using only one method?
void t1Pressed(){
  if(currentPosition == 0) {
    Timer1.stop();
    Timer1.detachInterrupt();
    noInterrupts();
    score++;
    guessPhaseFinished = true;
    interrupts();
    Serial.println("New point! Score: " + String(score));
    for(int x=0; x<NUM_LEDS; x++) {
      disableInterrupt(buttonPin[x]);
    }
    loopState = 7;
  }
}

void t2Pressed(){
  if(currentPosition == 1) {
    Timer1.stop();
    Timer1.detachInterrupt();
    noInterrupts();
    score++;
    guessPhaseFinished = true;
    interrupts();
    Serial.println("New point! Score: " + String(score));
    for(int x=0; x<NUM_LEDS; x++) {
      disableInterrupt(buttonPin[x]);
    }
    loopState = 7;
  }
}

void t3Pressed(){
  if(currentPosition == 2) {
    Timer1.stop();
    Timer1.detachInterrupt();
    noInterrupts();
    score++;
    guessPhaseFinished = true;
    interrupts();
    Serial.println("New point! Score: " + String(score));
    for(int x=0; x<NUM_LEDS; x++) {
      disableInterrupt(buttonPin[x]);
    }
    loopState = 7;
  }
}

void t4Pressed(){
  if(currentPosition == 3) {
    Timer1.stop();
    Timer1.detachInterrupt();
    noInterrupts();
    score++;
    guessPhaseFinished = true;
    interrupts();
    Serial.println("New point! Score: " + String(score));
    for(int x=0; x<NUM_LEDS; x++) {
      disableInterrupt(buttonPin[x]);
    }
    loopState = 7;
  }
}

void timerEnded() {
  Timer1.detachInterrupt();
  noInterrupts();
  guessPhaseFinished = true;
  interrupts();
  for(int x=0; x<NUM_LEDS; x++) {
      disableInterrupt(buttonPin[x]);
  }
  loopState = 8;
}

void increaseDifficult(){
  ballSpeed = ballSpeed * (1 - COMPLEXITY_FACTOR);                  //Augmenting the ball speed by 10%
  currentTime = currentTime * (1 - difficult * COMPLEXITY_FACTOR);  //Reducing the time to press the button bu the factor F (being affected by the diffuclt set at the beginning);
  loopState = 4;  
}

void gameOver(){
  Serial.println("Game Over. Final Score: " + String(score));
  turnOffAllLEDs();
  delay(10000);     //Wait 10 seconds
  loopState = 0;
}

void setup(){
  loopState = 0;
  sleeping = false;
  newGameRequested = false;
  currentTime = STARTING_TIME_IN_MILLIS;
  ballSpeed = GREEN_LED_DELAY;
  brightness = 0;
  fadeAmount = 5;
  for(int x=0; x<NUM_LEDS; x++) {     //Settings LEDs to OUTPUT and buttons to INPUT
    pinMode(ledPin[x], OUTPUT);
    pinMode(buttonPin[x], INPUT);
    turnOffAllLEDs();
  }
  pinMode(RED_LED, OUTPUT);
  randomSeed(analogRead(POT));       //I define the random seed using current value of the potentiometer
  Timer1.initialize();
  Serial.begin(9600);
  }

void loop(){
  switch(loopState) {
    case 0: //Starting phase
      initialState();
    break;
    case 1: //Red LED fading, waiting for a new game
      waitForNextGame();
    break;
    case 2: //Deep sleeping
      sleepNow();
    break;
    case 3: //New game
      newGame();
    break;
    case 4: //Game in progress
      gameLoop();
    break;
    case 5: //Guess the position
      makeGuess();
    break;
    case 6: //System wait for guess (until time is over)
      waitForGuess();
    break;
    case 7: //Correct guess -> increase the difficult before another guess
      increaseDifficult();
    break;
    case 8: //Wrong guess -> game over
      gameOver();
    break;
  }
}
