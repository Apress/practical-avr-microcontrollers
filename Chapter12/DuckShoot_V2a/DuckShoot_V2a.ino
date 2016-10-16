//
// This code is for the game duckshoot:
/* 
 This game uses a row of LEDs and a "fire" button. The LEDs (of which there are
 always an odd number, with the centre LED being the target LED for the fire
 button) flash on and off in a rotating pattern. i.e. the on/off pattern
 circulates from right to left. The game can present to the user in three modes. 
 
 In gameMode = 0: The game initialises with just the centre LED on, and after
 the LED pattern starts circulating, the fire button inverts the state of the centre
 LED, the idea being for the user to fire when the ON state LED is in the firing line.
 If the user "misses" the ON LED, then two LEDs will be on - and so on. Only when all
 LEDs are out is the game over. A new round of the begins after a few seconds, but this time
 the LEDs will circulate a little bit faster...... at the start of each round the LEDs 
 all come on, and wipe downwards (RtoL) to show the speed level. 
 
 In gameMode=1: Similar to mode 0 except that, at startup time ALL the LEDs except the
 firing line LED start as ON. 
 
 in gameMode=2: The LEDs on either side of the firing line LED are set to random
 patterns. When the two clusters contain the same pattern, the user should press fire.
 if the user was correct, then the patterns flash, if not then a "failed" display
 indicates that.
 
 The game mode is changed when the user presses the firing button down for more than 10 
 seconds. 
 
 Can you implement more modes yourself......?
 
 NOTE: All timings in this game rely on using the AVR chip at 8Mhz. You'll have to make
 adjustments to timing loops throughout if you use a different click rate. 
 
 */

// The major and minor version numbers
#define VERSMAJ "2.0"
#define VERSMIN "a"
#define DUCK_COUNT 11 //* How many ducks are there?
#define GUN_BUTTON 1 // Define which analog input port the fire button is on. 
#define GUN_BUTTON_LED 2 // Define which digital output port drives the LED inside the Gun Button. 
#define FIRING_LINE 8 // Which Pin drives the firing line LED?
#define LOW_LED 3 // What's the bit number of the least significant (visually rightmost) duck? 
#define HIGH_LED 13 // What's the bit number of the most significant (visually leftmost) duck? 
#define FIRING_LINE_BIT_NUM (DUCK_COUNT-1)/2 // What bit number represents the firing line? 
#define GBL_FLASH_RATE 100 // Milliseconds between state changes when the Gbl is flashing. 
#define MAX_SPEED_MS 90 // Max number of milliseconds between LED anims (sets slowest speed).
#define MIN_SPEED_MS 10 // Min millisecs beween LED anims (determines fastest speed)
#define SPEED_STEP (MAX_SPEED_MS - MIN_SPEED_MS)/DUCK_COUNT // How many milliseconds do we shave off for each speed level?

#define Debug true

// Global variables.
unsigned long ducks = 0;
int intPinNum = 0; 
int myLoop = 0 ;
int speedLevel = 1; // From 1 to DUCK_COUNT speed levels are to be available.
int levelMissCount=0;
int overallMissCount=0;
int levelHitCount=0;
int overallHitCount=0;

boolean GblArmed = false; // Controls whether button LED flashes or not. 
int GblCycles=0;
unsigned long buttonDownStart;
unsigned long buttonDownDuration;

void setup()
{
  Serial.begin(9600);

  pinMode(GUN_BUTTON_LED,OUTPUT);
  SetGunButtonLedState(0); // Remember low means LED comes on

  // The pin numbers LOW_LED to HIGH_LED are setup as outputs
  int MyLoop=0;

  do {
    pinMode(LOW_LED+MyLoop,OUTPUT);
    MyLoop++;
  } 
  while (MyLoop <= HIGH_LED);
  // Init the ducks variable
  /*
  ASCII Diagram time!
   |---|---|---|---|---|---|---|---|---|---|---|
   |L10|L09|L08|L07|L06|L05|L04|L03|L02|L01|L00| LED Number (same as physical layout on board)
   |P13|P12|P11|P10|P09|P08|P07|P06|P05|P04|P03| Arduino port number
   |B10|B09|B08|B07|B06|B05|B04|B03|B02|B01|B00| Bit number in Duck variable that holds LED status
   |---|---|---|---|---|!!!|---|---|---|---|---| !!! indicates firing line LED. 
   */
  digitalWrite(FIRING_LINE,LOW);

  resetDucks(0);
  showSpeedLevel(speedLevel); // Show we're on level one. 
}

void loop()
{
  doGblFlash(); // Go progress the Gun Button LED flasher. It just returns if inactive. 

  // Check see if the button is pressed. 
  if (analogRead(GUN_BUTTON)>=100) {
    // Yes it is. Debounce delay
    buttonDownStart=millis();
    delay(5);

    // Give the player instant feedback. 
    // If the firing line LED was ON it goes off, and vice versa
    if (digitalRead(FIRING_LINE)==HIGH) {
      // It was high, the LED was off. They missed 8-(
      for (int i=0;i<=20;i++){
        // Quickly fade in the LED to indicate the miss.
        digitalWrite(FIRING_LINE,LOW);
        delay(i/4);
        digitalWrite(FIRING_LINE,HIGH);
        delay(20-i);
      } 
      digitalWrite(FIRING_LINE, LOW); /* It's on */
      bitSet(ducks,FIRING_LINE_BIT_NUM);    
      // Bump the counters.
      levelMissCount++;
      overallMissCount++;
    }
    else {
      // The LED was on, they hit! 
      /* for (int i=0;i<=10;i++){
       // Quickly fade out the LED to indicate the hit.
       digitalWrite(FIRING_LINE,HIGH);
       delay(i+10);
       digitalWrite(FIRING_LINE,LOW);
       delay(10-i);
       } */
      doFadeOutLed(FIRING_LINE);

      digitalWrite(FIRING_LINE,HIGH); 
      bitClear(ducks,FIRING_LINE_BIT_NUM); 
      levelHitCount++;
      overallHitCount++;
      // Are all the duck lights off now?
      if (ducks==0) { 
        // They have completed this level. Move them to the next.
        Serial.print("Level ");
        Serial.print(speedLevel,DEC);
        Serial.print(" completed. Hits/Misses: ");
        Serial.print(levelHitCount,DEC);
        Serial.print("/");
        Serial.println(levelMissCount,DEC);

        overallHitCount+=levelHitCount;
        overallMissCount+=levelMissCount;
        levelHitCount=0;
        levelMissCount=0;
        speedLevel++;
        if (speedLevel>DUCK_COUNT) {
          weHaveAWinner();
          speedLevel=1; // Reset the speed level
        }
        showSpeedLevel(speedLevel);
        resetDucks(0);
        // And off we go again.  
      } 
      else
      {
        // They hit one, but there are more
        GblCycles=15; // Flash the Gbl to show they hit. 
        GblArmed=true;
      }
    }
    // Await button release by user. 
    do 
    {
      doGblFlash(); // Go progress the Gun Button LED flasher. It just returns if inactive. 
#ifdef Debug
      // A debug feature. If they hold down the button for more than 10 seconds they win the game. 
      buttonDownDuration=millis() - buttonDownStart;
      if (buttonDownDuration > 10000){
        speedLevel=1;
        weHaveAWinner(); // Do the winner's light show. 
        showSpeedLevel(speedLevel);
        resetDucks(0);  // Ducks start off as only centre one on.
      }
#endif     
    } 
    while (analogRead(GUN_BUTTON)>=100) ;


  } 
  AnimLeds(MAX_SPEED_MS - (speedLevel-1)*SPEED_STEP);
} 

void showSpeedLevel(int currentLevel) {

  /* This masterpiece shows - on the LEDS - the current speed level of the game.
   It first turns on all the LEDs, then it peels them down from right to left
   until the number of ON LEDS is the same as the current speed level. Then, it 
   pauses, turns off all the LEDs and exits. 
   */
  unsigned long ledRegister =0xFFFF; // LED reg starts off as all ones.
  int c=0;

  // Quickly turn all LEDS off to start with 
  turnAllLedsOff(0);

  // Then artistically turn them all on LTR
  for (c=0;c<4;c++){
    turnAllLedsOn(15);
    turnAllLedsOff(0);
  }
  turnAllLedsOn(30);
  c=0;

  // Now pare down the ON leds to indicate the required level.
  while((DUCK_COUNT - c) > currentLevel)
  {
    digitalWrite(c+LOW_LED,HIGH);
    c++;
    delay(100);
  } 

  // 3 seconds gawping time for the early rounds, but less when they have seen what to do. 
  if (currentLevel<=3){
    delay(3000);
  } 
  else
    delay(1500);


  // And we're gone. Turn out the lights.
  turnAllLedsOff(0);
}

void turnAllLedsOn(int dly) {
  /* Remember in this code that LOW means ON and HIGH means off
   because the ports are sinking current through the LEDs 
   rather than sourcing it. */
  int c=0;
  // Turn all the LEDs ON Right to Left with the requested artistic delay.
  // (can be zero if no delay required).
  for(c=DUCK_COUNT;c>=0;c--) {
    digitalWrite(c+LOW_LED,LOW); 
    delay(dly);
  }
}

void turnAllLedsOff(int dly) {
  /* Remember in this code that LOW means ON and HIGH means off
   because the ports are sinking current through the LEDs 
   rather than sourcing it. */
  int c=0;
  // Turn all the LEDs OFF Right to Left 
  for(c=DUCK_COUNT;c>=0;c--) {
    digitalWrite(c+LOW_LED,HIGH); 
    delay(dly);
  }
  return;
}

void flashAlternates(int dly, int altCycles, boolean varyTiming ){
  /* In this code LOW means the LED is ON. 
   This code flashes the two groups of LEDs to the left and right
   of the firing line alternately. */
  if (altCycles <0) {
    return;
  }
  int i,j,k,blockOneStart, blockOneEnd, blockTwoStart, blockTwoEnd = 0;
  turnAllLedsOff(0);

  /* Initially, turn everything to the right of the firing line ON. */
  for (i=FIRING_LINE+1;i<=HIGH_LED;i++){
    digitalWrite(i,LOW); 
  }

  for(i=0;i<altCycles;i++){
    for (k=LOW_LED;k<=HIGH_LED;k++){
      if (k!=FIRING_LINE) {
        if (digitalRead(k) == HIGH) {
          digitalWrite(k,LOW); 
        }
        else{
          digitalWrite(k,HIGH); 
        }             
      }
    }
    if ((varyTiming==true) && (dly >=20)){
      dly=dly-(altCycles/50); 
    }
    delay(dly);
  }
}

void SetGunButtonLedState(int NewState){

  // This routine sets the state of the LED inside the fire
  // button. The sent value of NewState determines what happens
  // 0=off 1=on 2=invert current state
  switch  (NewState) {
  case 0:
    digitalWrite(GUN_BUTTON_LED,LOW); // Remember that low means "on" when ports sink LED current
    break;
  case 1:
    digitalWrite(GUN_BUTTON_LED,HIGH); // High means off when current sinking. 
    break;
  default: // Default state is to invert it. 
    if (digitalRead(GUN_BUTTON_LED)==LOW){
      digitalWrite(GUN_BUTTON_LED,HIGH);  
    } 
    else{       
      digitalWrite(GUN_BUTTON_LED,LOW); 
    }
  }
}


void doGblFlash() {
  // This little gem is activated if GblArmed boolean is true.
  // When active it inverts the state of the Gun button LED (Gbl) GblCycles times,
  // at a rate of 200ms timed against the system clock. 
  // In order to activate the Gbl the main loop sets thethe value of GblCycles and
  // sets GblArmed=true when the required flash is done this function resets
  // these variable to their inactive state. 

  static unsigned long  gblTimer=0; // This needs to be static so its value persists between calls

  if (GblArmed==false) {
    return; 
  }
  // Has 200ms elapsed since we last activated? 
  if ( (millis() - gblTimer) >=GBL_FLASH_RATE) {
    // It's time to flash. 
    if ((GblCycles--)<=0) {
      // We've done the required number of flashes. Go inactive.
      GblArmed=false;
      gblTimer=0;
      SetGunButtonLedState(0); // LED goes steady ON (assumes sinking LED current). 
      return;
    }
    else {  
      // Okay, we're still here - more flashin' to be done. 
      SetGunButtonLedState(2); // invert the state of the Gbl
      gblTimer=millis(); // Remember when we did this flash.
      return;
    }
  }
}

void AnimLeds(int dValue ){
  // In this function we progress the animation of the LEDs by one position according to the
  // mode of the game. We incorporate the indicated delay value. 

  int DuckLo=0;
  unsigned long notDucks=0;
  int thisPinNum = 0 ;
  // Wait for the desired number of milliseconds. 
  delay(dValue); 
  //   Serial.println("Starting AnimLeds");

  DuckLo=bitRead(ducks,0); // Save the lowest LED state
  ducks=ducks/2;           // Move the state of all ducks one bit to the right
  // Now put the state that was at the rightmost LED onto the leftmost. 
  if (DuckLo==1) {
    bitSet(ducks,DUCK_COUNT-1);
  } 
  else {
    bitClear(ducks,DUCK_COUNT-1);
  }
  // Now output ducks to the LEDs. The LEDs are ON when port is low, so we
  // actually invert the ducks for output. 

  thisPinNum =LOW_LED;
  notDucks=~ducks;

  do {
    digitalWrite(thisPinNum,bitRead(notDucks,thisPinNum-LOW_LED));
    thisPinNum++;
  }
  while(thisPinNum <= HIGH_LED);

}

void weHaveAWinner(){
  // Wow! That's the end of the game, they have conquered the highest level. Show them
  // the flash alternates display to indicate their greatness. 
  long unsigned thisTime;

  Serial.println("All Levels completed");
  Serial.print("Overall Hits/Misses: ");
  Serial.print(overallHitCount,DEC);
  Serial.print("/");
  Serial.println(overallMissCount,DEC);
  overallHitCount, overallMissCount=0;  

  doEndTogetherLeds(10,30);
  flashAlternates(150,100,true);
  doFadeOutLed(0);

  thisTime=millis();
  do {
    // Go round this loop for about four seconds or so.
    turnAllLedsOn(3);
    delay(100);
    doFadeOutLed(0);
    delay(125);
  } 
  while(millis() < (5000+thisTime));


  turnAllLedsOff(5);
  delay(4000);
  resetDucks(0);
  speedLevel=1; /* Start again. */
}

void resetDucks(int Mode){
  switch (Mode ) {

  case 0:
    ducks= pow(2,FIRING_LINE-LOW_LED);  // Ducks start off as only centre one on. 
    break;
  } 
}


void doFadeOutLed(int PinNum) {
  // This function fades out the LED connected to the indcated pin number.
  // If the specified pin is == 0 then we fade out ALL LEDs. Remember that
  // due to theconnection used for LEDs on the game LOW means ON and HIGH
  // means OFF. It woudl be ideal to do all these pins with PWM, but only 
  // a few support it, so this is the coding way to do it. 
  //
  int StartPin, EndPin=0;
  int Lp =0;

  if (PinNum==0) {
    // We are doing all LEDs
    StartPin=LOW_LED;
    EndPin=HIGH_LED;
  } 
  else
  { 
    // We're doing just one LED. 
    StartPin=PinNum;
    EndPin=PinNum;
  }

  for (int i=0;i<=10;i++){
    Lp=StartPin;
    do {
      // Put the LED(s) ON
      digitalWrite(Lp,LOW);
      Lp++;
    } 
    while(Lp<=EndPin);
    delay(10-i); // ON time decreases as i gets bigger
    Lp=StartPin;
    do {
      // Switch the LED(s) OFF
      digitalWrite(Lp,HIGH);
      Lp++; 
    }
    while(Lp<=EndPin);

    delay(i+10); // OFF time increases as i gets bigger with each loop. 

  }
}



void doEndTogetherLeds(int Iterations, int StepTime){
  int Lp,Lp1,Lp2,Lp3=0;

  Lp=HIGH_LED;
  Lp1=LOW_LED;
  Lp3=Iterations;

  do {
    for (Lp2=0;Lp2<(DUCK_COUNT/2);Lp2++){
      turnAllLedsOff(0);
      digitalWrite(Lp2+LOW_LED,LOW);
      digitalWrite(HIGH_LED-Lp2,LOW);
      delay(StepTime);

    }
    turnAllLedsOff(0);
    digitalWrite(FIRING_LINE,LOW);
    delay(StepTime*2);
    Lp3--;
  }
  while (Lp3>0);

}


















































