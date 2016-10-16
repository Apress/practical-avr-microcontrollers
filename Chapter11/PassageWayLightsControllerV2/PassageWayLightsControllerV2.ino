
/*
 This software controls a number of LED lights (more usually LED strings) placed along
 a passageway, or corridor. A PIR sensor at each end of the passageway feeds into the 
 hardware which this software controls, to tell it that someone has entered one or other
 end of the lit space. The software then fades up the lights starting 
 from the end of the space where the person entered. Some (configurable) time later, it
 fades the lights down again in the same direction to that in which they were put on.
 
 The sofware has the ability to accept external commands received as strings via the serial 
 channel. These are:
 
 "ALLON"All the lights come on at full PWM intensity.
 "ALLOFF" All the lights go off.
 "BEEP" Make a bleep noise.
 "CLOCK" Show the number of milliseconds since the program started.
 "CYCLE" Do one minute of alternating fades between odd and even lights
 "FDUP" Fade all lights up to full
 "FDWN" Fade all lights down to dark
 "Ixxxx" All the lights come on at PWM intensity xxxx (0 - max PWM)
 "SPP" Show current values of main parameters from the lights array.
 "Txx" Simulate a sensor trigger from sensor xx 
 
 The hardware provides a control button to change the mode between Auto, ON and OFF. 
 It also provides a small internal sounder/speaker the makes a sound that
 provides an indication of the current mode. An LED connected to LEDPIN blinks
 to show activity.
 
 */


const char* VERSION_STRING = "Passageway Lights Controller: Version 2.0a 03/05/2012";
const char* PROMPT = "Cmd>";

#include "Tlc5940.h"
/*
 Instead of using Arduino's PWM pins, this version uses a TLC5940 chip to 
 provide 16 PWM channels. These are driven by the TLC5940 library code by
 Alex Leone. This TLC version probably contains a lot of duplication that
 could be stripped out, it's offered as a starting point.
 
 The control routines are called from the loop() and some are not tied to the
 system clock, they just happen as often as possible. This will necessitate
 changes to some software timing if used on a setup that uses something other
 than the 8Mhz speed used in development.
 */

// Constant Pin values (see also lights[] array definitions).
const int LEDPIN = 2;
const int BUTTON = 5;
const int SENSOR1= 6;
const int SENSOR2 = 7;
const int SOUNDER = 4;
const int HH_SENSE = A5; // Jumper sense input to see if we are participating in Home Help.

// What will the sensor pin values return when the sensors is NOT activated?
const int SENSOR1_INACTIVE = LOW; 
const int SENSOR2_INACTIVE = LOW; 

/*
 The lights come on and turn off in a waterfall sequence, starting from the direction of
 trigger. If the trigger comes from sensor1 then the lowest number light is on first and
 off first. If the trigger comes from sensor2, then the highest numbered light is on first
 and off first. The following parameter sets the speed of the waterfall, setting
 how many milliseconds between each increment of the waterfall effect. 
 This needs to be an unsigned long to reliably add to clock vals.
 */
// >>>> ADJUST THIS VALUE TO COMPENSATE FOR FEWER OR MORE LIGHTS  <<<<<
const unsigned long  SEQUENCE_STAGGER = 200; 

/*
  Sensor parameters.
 */
const byte SENSOR_COUNT = 2;

/* 
 Light Mode values. These are the possible modes in which each light
 is animated by the main program loop and the hundredths function.
 Not all modes are implemented.
 */
const int OFF = 0;
const int ON = 1;
const int FADEIN = 2;
const int FADEOUT = 3;
const int FADECYCLE = 4;
const int FLASH_SLOW = 5;
const int FLASH_MED = 6;
const int FLASH_FAST = 7;
const int NONE = 8;

struct MODE_DEFINITIONS {
  int modeValue;
  char* modeText;
  boolean isEnabled;
};

const MODE_DEFINITIONS modeNames[]= { 
  OFF, "OFF", true,
  ON, "ON", true,
  FADEIN, "FadeIn", true,
  FADEOUT, "FadeOut", true,
  FADECYCLE, "FadeCycle", true,
  FLASH_SLOW, "SlowFlash", true,
  FLASH_MED, "MediumFlash", false,
  FLASH_FAST, "FastFlash", false, 
  NONE,"NONE", true};

int maxPWMValue = 4095;
unsigned long val;

/*
  PWM consists of chopping up time into slices, and putting something (usually a light or a motor)
 "on" for part of each slice, and "off" for the rest of it. If you do this fast enough, an increased
 "On" time results in a light that runs brighter or a motor that turns closer to its max speed.
 If you decrease the "on" time, you get a light that runs dim, or a motor that turns more slowly. 
  
 Lights are described and controlled by the lights structure whose definition now follows
 */

// Define the data structures concerned with the lights.
struct LIGHT_STRUCT{
  int pinNumber;                  // Specifies the 5940 pin number to which the light (or light driver) is attached
  int PWMValue ;                  // Contains this light's PWM value from 0 (off) to 4095 (full on).
  int pendingMode;                // The mode this light shall enter when it's pending countdown reaches zero
  unsigned long modeChangeTime;   // The clock time at which the light shall enter a new mode at a future time
  unsigned long turnOffTime;      // The clock time at which this light is to be turned off. If zero, means don't check.
  int subState;                   // For compound modes such as FadeCycle we use this to show the current substate
  int currentMode;                // Which mode is this light running in?
};

LIGHT_STRUCT lights[]= {
  0,0,NONE,0,0,NONE,OFF,
  1,0,NONE,0,0,NONE,OFF,
  2,0,NONE,0,0,NONE,OFF,
  3,0,NONE,0,0,NONE,OFF,
  4,0,NONE,0,0,NONE,OFF,
  5,0,NONE,0,0,NONE,OFF,
  6,0,NONE,0,0,NONE,OFF,
  7,0,NONE,0,0,NONE,OFF,
  8,0,NONE,0,0,NONE,OFF,
  9,0,NONE,0,0,NONE,OFF};  

const int LIGHT_COUNT = 10;  // How many lights are there in this setup?

const unsigned long LIGHTON_DEFAULT_DURATION = 40000; // How many millisecs do the lights stay on?
const unsigned long FADECYCLE_SEQUENCE_LENGTH = LIGHTON_DEFAULT_DURATION * 1.5  ; // How many milliseconds does the fade cycle sequence run for?
const int FADE_CYCLE_INCREMENT_VALUE = 50;

boolean longPressBeepDone = false;
char buffer[100];
char inputBuffer[8];
int inputBufferPtr = 0;

/*
  Possible program modes.
 */
const byte NORMAL =0;
const byte ALLON = 1;
const byte INACTIVE = 2;
const byte NO_MODE = 3; 
/*
  NO_MODE is used to show the mode advancer that we have reached the end of the mode list
  Insert new modes if you want, but always make NO_MODE the last and highest numbered.
  */

/*
  Press button handling.
 */
const unsigned long LONG_BUTTON_PRESS_MS = 7000; 
// In milliseconds, how long does the button have to be pressed to count as a long press?

/*
  HOME HELP Support variables.
 */
boolean PROVIDE_MONITORING_INFO = false;
const int unitID = 0x11;
const long HH_TAXONOMY_EVENTS_PIRSENSOR_TRIGGER = 0x01010500;

byte currentProgramMode = NORMAL;


void setup(){

  // Init the TLC5940
  Tlc.init(0);
  Tlc.update();

  // Do the command button input.
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH); // Activate the pull up on the button input

  // Now the sensors.
  pinMode(SENSOR1,INPUT);
  pinMode(SENSOR2,INPUT);
  digitalWrite(SENSOR1, HIGH); // Activate the pull ups
  digitalWrite(SENSOR2, HIGH);

  // Init the sounder and do a startup sound.
  pinMode(SOUNDER,OUTPUT);

  // Init the LED pin.
  pinMode(LEDPIN, OUTPUT);

  // HH Sense pin.
  pinMode(HH_SENSE,INPUT);
  digitalWrite(HH_SENSE,HIGH);
  delay (20);

  // See if we need to output home help stuff on triggers.
  if (digitalRead(HH_SENSE)== LOW){
    PROVIDE_MONITORING_INFO=true;
  }

  // Tell anyone who cares that we are in here...
  Serial.begin(9600);
  sprintf(buffer,"\n\n\r%s\n\r%s",VERSION_STRING,PROMPT);
  Serial.print(buffer);
  doPowerUpSound();
}

void loop() {

  unsigned long lastHundredthSec = 100; 
  unsigned long buttonPressedTime =0; // At what time did we fist see the button pressed? 0 means it's not pressed now.

  /* 
  The main loop lasts forever because it's all inside this do {} while  loop
  */
  do {
    doSerialInput();

    // Button and sensor detect section.Button first.
    if (digitalRead(BUTTON) == HIGH) {
      // The button is not pressed. Was it recently pressed?
      if (buttonPressedTime != 0) {
        // It was pressed before, but now it's released. Action it according to how
        // long it was pressed for. 
        delay(100); // Debounce.
        if (( millis() - buttonPressedTime) >= LONG_BUTTON_PRESS_MS){
          longPress(millis() - buttonPressedTime); // Call longPress handler, tell it how long.
        }
        else
        {
          shortPress();  // If it wasn't a long press it was a short one: Advance the program mode.
        }
        buttonPressedTime = 0; // Clear it cos we actioned it someway. 
      }       
    }
    else
    {
      if (buttonPressedTime == 0) { // Button was pressed. First time we've seen it so? 
        delay(100);
        buttonPressedTime = millis(); // Yes, remember the time.
      }
      else
      {
        // If it's been held long enough for a long press then bleep
        if ((longPressBeepDone == false) && ( millis() - buttonPressedTime) >= LONG_BUTTON_PRESS_MS){
          doBeep();
          longPressBeepDone=true;
        }
      }
    }


    /*
     Now check and action the sensor inputs. The sensors used in the proto have
     normally closed contacts, which go open when movement is detected near them.
     Since they are normally connected to ground, that are active HIGH.
     We only respond to these in NORMAL operation mode.
     */
    if (currentProgramMode == NORMAL) {
      if (digitalRead(SENSOR1) != SENSOR1_INACTIVE) {
        sensorEvent(1);
      }

      if (digitalRead(SENSOR2) != SENSOR2_INACTIVE) {
        sensorEvent(2);
      }
    }

    // Main light processing loop. Calls hundredthSec as needed
    for (int l=0;l<LIGHT_COUNT;l++){

      // Write the PWMvalue for this pin out.
      if (lights[l].currentMode == OFF) {
        // Enforce lights out!
        Tlc.set(lights[l].pinNumber,0);
      }
      else
      {
        Tlc.set(lights[l].pinNumber,lights[l].PWMValue);
      }
      //Process the pending state for this light.
      if ((lights[l].modeChangeTime !=0 ) && (millis() > lights[l].modeChangeTime)) {
        // Time to change mode.
        lights[l].currentMode = lights[l].pendingMode;
        lights[l].modeChangeTime = 0;
        lights[l].pendingMode = NONE;
      }

      // Process the turn off time for this light.
      if ((lights[l].turnOffTime !=0) && (millis() >= lights[l].turnOffTime)) {
        lights[l].turnOffTime = 0;
        lights[l].currentMode = FADEOUT; // From whatever brightness it is now it fades out
      }
    }
    Tlc.update();
    // See if the HundredthSec timer needs firing 
    if ((millis() - lastHundredthSec) >=10)
    {
      // Yes it does. Go for it.
      lastHundredthSec = millis(); // Remember this fire time.
      hundredthSec();
    }   
  }
  while(1);
}

void oneSec() {
  if (currentProgramMode != INACTIVE){
    if (digitalRead(LEDPIN)==HIGH){
      digitalWrite(LEDPIN, LOW);
    }
    else
    {
      digitalWrite(LEDPIN,HIGH);
    }
  }
  else
  {
    digitalWrite(LEDPIN,LOW);
    delay(10); // We just blip the active LED very slightly in INACTIVE mode.
    digitalWrite(LEDPIN,HIGH);
  }
}

/*
 hundredthSec: We come here every 100th of a second from loop().
 This acts as a dispatcher for once per second events and for lights processing
 events - fade progressions and control changes etc. 
 */

void hundredthSec(){

  static unsigned long lastHundredthsRun = 0;
  static int hundredths = 100;

  // Stuff that we do every time we come here.

  // See if one second has elapsed.
  if (--hundredths == 0) {
    hundredths=100;
    oneSec();
  }

  // Now, advance fade states - step thru the array.
  for (int x = 0; x< LIGHT_COUNT; x++) {

    // What we do now depends on the mode of this light.
    switch (lights[x].currentMode) {

    case FADEIN:
      // If we are fading in, we bump the PWM value: If it's maxed we
      // change the mode to ON so we don't waste time fading it next time
      if ((lights[x].PWMValue+=20) >= maxPWMValue){
        lights[x].currentMode = ON;
        lights[x].PWMValue = maxPWMValue;
      }
      break;

    case FADEOUT:
      // If we are fading out, we look at the change value and if it's not
      // already at zero or less, we decrement it. If it's already zero we
      // change the mode to OFF
      if ((lights[x].PWMValue-=20) <= 0){
        lights[x].currentMode = OFF;
        lights[x].PWMValue =0;
      }
      break;

    case FADECYCLE:
      /*
        In a fade cycle mode we fade the light up, then down, then up etc. 
       Since the mode is always FadeCycle, we use the subState slot in the
       lights data structure to show which we are doing at the moment.  
       */
      if (lights[x].subState == FADEIN) {
        lights[x].PWMValue+=FADE_CYCLE_INCREMENT_VALUE;
        if (lights[x].PWMValue >=maxPWMValue){
          lights[x].subState=FADEOUT;
          lights[x].PWMValue=maxPWMValue;
        }
      }
      else
      {
        lights[x].PWMValue-=FADE_CYCLE_INCREMENT_VALUE;
        if (lights[x].PWMValue <=0) {
          lights[x].subState=FADEIN;
          lights[x].PWMValue=0;
        }
      }

      break;

    case OFF:
    case ON:
    default:
      break; // In these cases, do nothing

    }
  } 
}  


int doInputBufferParser() {

  boolean result = false;

  // This processes the command buffer. Commands may have come from serial channel
  // or (later) from the network.
  int y = 0;
  switch  (inputBuffer[0]) {

    // We used to have a HELP command here, but it exhausted the RAM, so had to remove it.
  case 'A':
    if (strcmp(inputBuffer,"ALLON")==0) {
      turnAllOn();
      result=true;
    }

    if (strcmp(inputBuffer,"ALLOFF")==0) {
      turnAllOff();
      result=true;
    }    
    break;


  case 'C': // Cycle command (puts all into FADECYCLE mode);
    if (strcmp(inputBuffer,"CYCLE")==0) {
      result = setFadeCycleMode();
      if (result == false) {
        Serial.println("Lights busy. Cannot set to cycle.");
      }
    }

    if (strcmp(inputBuffer,"CLOCK") ==0) {
      showSystemTime();
      result=true;
    }

    break;

  case 'I':
    // This has to be the only command starting with I.
    inputBuffer[0] =  '0';
    y=atoi(inputBuffer);
    if ((y>=0) && (y <= maxPWMValue)) {
      for (int x=0;x<LIGHT_COUNT;x++){
        lights[x].PWMValue = y;
        lights[x].currentMode=ON;
      }
      result = true;  
    } 
    break;

  case 'B':
    if (strcmp(inputBuffer,"BEEP")==0){
      doBeep();
      result=true;
    }
    break;

  case 'F':
    // It's a fade command. Is it 'FUP' or is it 'FDN'?
    // If all is as it should be, we should terminate after three chars
    for (y=0;y<LIGHT_COUNT;y++){
      if (strcmp(inputBuffer,"FDN")==0) {
        lights[y].PWMValue=maxPWMValue;
        lights[y].currentMode=FADEOUT;
      }
      if (strcmp(inputBuffer, "FUP") ==0) {
        lights[y].PWMValue=0;
        lights[y].currentMode=FADEIN;
      }
    }
    result=true;
    break;

  case 'S':
    // It's a show PWM params command. 

    if (strcmp(inputBuffer,"SPP")==0){
      showPwmParameters();
      result=true;
    }
    break;

  case 'T':
    inputBuffer[0] =  '0';
    y=atoi(inputBuffer);

    if ((y > 0 ) && (y < 3)) {
      result=sensorEvent(y);
      if (result == false) {
        Serial.println("Lights already in use.");
      }
    }
    break;

  default:
    Serial.print("Unknown command: ");
    Serial.println(inputBuffer);
    result = true;
    break;
  }
  // If we went into one of the command handlers, but could not execute it, or if the first letter in
  // the buffer does not match a known command, we get result == false.
  if (result==false) { 
    Serial.println("Cannot execute command");
  }
}

void showPwmParameters() {
  // This function outputs a summary of the PWM parameter values to the
  // serial channel in respnse to the spp command
  sprintf(buffer,"\n\rLights Status at ");
  Serial.print(buffer);
  showSystemTime();
  sprintf(buffer," # %07s %07s %12s %12s %010s %10s\n\r","MaxPWM", "PWM Now" , "Mode Now", "PendingMode", "Change at", "OffTime");
  Serial.print(buffer);

  for (int theLoop = 0; theLoop < LIGHT_COUNT; theLoop++){  
    sprintf(buffer,"%02d %07d %07d %12s %12s %010ld %010ld\n\r",theLoop+1,
    maxPWMValue,
    lights[theLoop].PWMValue,
    modeNames[lights[theLoop].currentMode].modeText,
    modeNames[lights[theLoop].pendingMode].modeText,
    lights[theLoop].modeChangeTime,
    lights[theLoop].turnOffTime);

    Serial.print(buffer);
  }
}

boolean sensorEvent(int sensorNum) {
  /*
   This function carries out the light sequencing when a sensor trigger is received.
   Triggers only have effect if the lights are currently OFF. The currentState of 
   light[0] is sampled to determine whether the lights are OFF at the time of the 
   trigger.
   
   The lights are programmed to fadein in a time-staggered sequence starting from
   now. The formula for setting future times is onTime=now+(light# * SEQUENCE_STAGGER)
   in milliseconds. If the sensorNum is 1 then the lights fade in the array order 0,1,2,3
   etc. If the sensor number is 2, then they switch on in descending order.
   */
  Serial.println("Sensor event.");
  Serial.println(sensorNum);

  int startLight, endLight, incValue, thisLight =0;

  if (currentProgramMode == INACTIVE) {
    return(false);
  }

  if (lights[0].currentMode != OFF) {
    return(false);
  }

  if (sensorNum ==1) {
    startLight = 0;
    endLight = LIGHT_COUNT;
    incValue=1;
  }
  else
  {
    startLight = LIGHT_COUNT -1;
    endLight = -1;
    incValue = -1;
  }
  thisLight=startLight;
  int ctr=0;

  do {
    // Set this light up.
    lights[thisLight].PWMValue=0; // The light intensity is 0
    lights[thisLight].currentMode=ON;  // The mode now is ON (but dimmed to 0)
    lights[thisLight].pendingMode=FADEIN; // We will enter FADE IN mode... 
    lights[thisLight].modeChangeTime = millis()+(ctr*SEQUENCE_STAGGER); // ... in this many milliseconds
    lights[thisLight].turnOffTime = lights[thisLight].modeChangeTime + LIGHTON_DEFAULT_DURATION; //We will turn off at this time.
    lights[thisLight].subState=NONE;

    ctr++;
    thisLight+=incValue;

  }
  while(thisLight != endLight);

  // If we are centrally monitoring with home-help provide the info to the serial channel xmitter.
  if (PROVIDE_MONITORING_INFO == true) {
    sprintf(buffer,"%x:%x:%x\n\r",unitID,HH_TAXONOMY_EVENTS_PIRSENSOR_TRIGGER,sensorNum);
    Serial.print(buffer);
  }
  return(true);
} 

void doBeep() {
  tone(SOUNDER,1000,500);
}

boolean setFadeCycleMode() {
  /*
  This function puts the lights into fade cycle mode for one minute.
   */
  if (currentProgramMode == INACTIVE) {
    return (false);
  }

  for (int t=0;t<LIGHT_COUNT;t++) {
    if (t & 1){
      // Odd numbered lights start on the downfade.
      lights[t].subState = FADEOUT;
      lights[t].PWMValue = maxPWMValue;
    }
    else
    {
      lights[t].subState = FADEIN;
      lights[t].PWMValue = 0;
    }
    lights[t].currentMode = FADECYCLE;
    lights[t].pendingMode = FADEOUT;
    lights[t].modeChangeTime = millis() + FADECYCLE_SEQUENCE_LENGTH;
    lights[t].turnOffTime = 0; //millis() + FADECYCLE_SEQUENCE_LENGTH +100;
  }
  return(true);
}



void longPress(unsigned long pressDuration) {
  // We don't use the duration at the moment, we may in future. For example a restart might be a 20 second
  // press. At the moment we just use it to kick off the prettys
  if (currentProgramMode == INACTIVE) {
    return;
  }

  boolean myRes = setFadeCycleMode();
  longPressBeepDone=false;
  return;
}

void shortPress() {
  /*
   This function advances the program mode by 1. 
   */
  if (++currentProgramMode == NO_MODE) {
    currentProgramMode = NORMAL; // Reset the mode if we reached the max.
  }

  switch (currentProgramMode) {

  case NORMAL:
    doPowerUpSound();
    break;

  case ALLON:
    turnAllOn();
    break;  

  case INACTIVE:
    turnAllOff();
    break;
  }
}

void doPowerUpSound() {
  for (int t=100; t<1100; t+=1){
    tone(SOUNDER,t,7);
    //delay (3);
  }
}

void doPowerDownSound() {
  for (int t=1000; t>200; t-=1){
    tone(SOUNDER,t,7);
    //delay (1);
  }
}  



void doSerialInput(){
  byte aChar;
  // Check for a Serial command.
  if (Serial.available() >0) {
    // get the char
    aChar=Serial.read();
    if (processKeyboardChar(aChar)){
      // It was a terminator. Tail the buffer and parse it
      Serial.println();
      inputBuffer[inputBufferPtr++]=0;
      doInputBufferParser();
      inputBufferPtr=0;
      inputBuffer[0]=0;
      Serial.print(PROMPT);
    }
  }
}

int processKeyboardChar(char theChar) {
  /*
    This function actions an input character. Buffers the char if it's
   a printable one. If it's a control char, then actions it or ignores
   it. If it's a CR char, then returns 1, else it returns zero.
   */
  if ((32==theChar) || (theChar=='*') || (isalnum(theChar))){
    /*
    This is a printable char just buffer and echo the char if that
     would not make the string too long. Convert to upper case if 
     alphabetic.
     */
    if (isalpha(theChar)){
      bitClear(theChar,5);
    }
    if (inputBufferPtr < sizeof(inputBuffer)){
      inputBuffer[inputBufferPtr++]=theChar;
      inputBuffer[inputBufferPtr]=0; 
      Serial.print(theChar);
      return(0);
    }
  }  
  else
  {
    // It's a control character
    switch  (theChar) {

    case 13: // Carriage return (new line)
      return(1);
      break;

    case 18: // CTRL/R
      Serial.println("^R"); // New line
      Serial.print(inputBuffer);
      break;

    case 8: // Backspace
      // Only process this if there is something in the buffer.
      if (inputBufferPtr > 0) {
        inputBuffer[--inputBufferPtr]=0; // Kill the char in the buffer.
        Serial.print(theChar); // Send the backspace on screen
        Serial.print(" ");
        Serial.print(theChar);
      }
      break;
    }
    return(0);
  }
}

void turnAllOn(){
  for (int t=0; t< LIGHT_COUNT;t++) {
    lights[t].currentMode=ON;
    lights[t].pendingMode=ON;
    lights[t].PWMValue=4095;
  }
  doBeep();
}



void turnAllOff(){
  for (int t=0; t< LIGHT_COUNT;t++) {
    lights[t].currentMode=OFF;
    lights[t].pendingMode=OFF;
    lights[t].PWMValue=0;
  }
  doPowerDownSound();
}


void showSystemTime() {
  sprintf(buffer,"System Time: %09ld\n\r",millis());
  Serial.print(buffer);
}









