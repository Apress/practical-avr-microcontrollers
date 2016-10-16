/*
 Secret Panel Controller: Version 1.0d
 See notes about this software in the book
 Practical AVR Microcontrollers.
 */

#define VERSION_STRING "Secret Sliding Panel Driver V1.0d 22-MAY-2012"

// Constants for I/O lines
#define PANEL_FULLY_CLOSED 2 
#define PANEL_FULLY_OPEN 3
#define MOTOR_ENABLE 5 // Set to HIGH to enable Motor dvr chip
#define MOTOR_DRIVE_A 6
#define MOTOR_DRIVE_B 7
#define USER_SWITCH 4
#define LED_STRINGS 9  // Use PWM capable pin for LEDs.
#define FAIL_LED 8 // LED that we put on if panel take too long

// Globals
const int LED_DELAY=5; // We use this to slow the LED fades as required

int limitSwStates[2];  // [0] Fully open limit sw [1] fully closed

const unsigned long MAX_PANEL_MOVE_TIME = 5000; // In milliseconds (ie seconds x 1000)


void setup() 
{
  // Get the motor control into a known state
  pinMode(MOTOR_ENABLE,OUTPUT);
  pinMode(MOTOR_DRIVE_A,OUTPUT);
  pinMode(MOTOR_DRIVE_B,OUTPUT);
  motorOff(); // All stop.

  // Setup the input pins
  pinMode(PANEL_FULLY_CLOSED, INPUT); // These are the limit switch
  pinMode(PANEL_FULLY_OPEN, INPUT);   // inputs
  pinMode(USER_SWITCH,INPUT);

  // Enable pull ups on all the inputs
  digitalWrite(PANEL_FULLY_CLOSED,HIGH);
  digitalWrite(PANEL_FULLY_OPEN,HIGH);
  digitalWrite(USER_SWITCH,HIGH);

  // The LED control lines
  pinMode(LED_STRINGS,OUTPUT);
  pinMode(FAIL_LED,OUTPUT);
  setFailLedState(false);

  // The serial port
  Serial.begin(9600);
  Serial.println(VERSION_STRING);

  // Get the initial limit states.
  limitSwStates[0] = digitalRead(PANEL_FULLY_OPEN);
  limitSwStates[1] = digitalRead(PANEL_FULLY_CLOSED);

  // Pulse the LEDs up and down really fast, to show they work
  for (int q=0;q<=255;q++)
  {
    progressLedPwm(true);
  }
  for (int q=0;q<=255;q++)
  {
    progressLedPwm(false);
  }

  if (limitSwStates[1] != LOW)
  {
    // Reset the panel to closed positio if it's not already.
    boolean res = closePanel(0);
  }
}

void loop()
{
  /*
   Here inside the main loop we simply scan for a user switch press.
   when we find one, we look at the limit switched and:
   
   if Limit switch 1 (PANEL_FULLY_CLOSED) is active we assume the panel is closed
   and we open it.
   
   if Limit switch 2 is active (or neither limit switch is active)
   we assume the panel is open and we close it.
   
   */
  if (returnSwitchState() == true)
  {
    /*
    Somebody pressed and then released the user switch
    react according to the state of the sensors.
    */
    if (digitalRead(PANEL_FULLY_CLOSED) == LOW) 
    {
      // The panel seems closed. Open it up.
      Serial.print("Panel Opening at ");
      Serial.println(millis());
      openPanel(millis()+10);
    }
    else
    {
      // The panel seems open or indeterminate. Close it (or try).
      Serial.print("Panel Closing at ");
      Serial.println(millis());
      closePanel(millis()+10);
    }
  }

  // Update the panel limit switch states
  updateLimitSwStates();
}



boolean closePanel(unsigned long ledFaderStart)
{
  /*
   Close panel. Set start time. Ramp up motor. Waits for 
   fully closed limit switch hit, or a timeout - whichever
   is the sooner. No fault detect code in here to check for
   a mechnical problem during the panel transit: The H chip 
   used (L293D) does not present any over current indications
   just shuts down, which will cause a timeout anyway.
   LED fading starts after ledFaderStart milliseconds. 
   */
  unsigned long startTime;
  boolean allOkay = false;

  startTime = millis();
  rampPanelMotorUp(MOTOR_DRIVE_A);

  /* 
   Now, wait for the panel to hit limit switch, or timout.
   while we wait , we fade out the leds.
   */

  do
  {
    if (millis() > ledFaderStart)
    {
      progressLedPwm(false); // This stops when it gets to zero.
    }
    updateLimitSwStates();
    if (limitSwStates[1]== LOW) 
    {
      motorOff();
      allOkay = true; // flag that we succeeded.
      Serial.println("Panel Fully Closed Sensor hit");
      break; // Break out of the do-while loop. We're done
    }
    // Not there yet.
  }
  while ((millis() - startTime) < MAX_PANEL_MOVE_TIME);

  setLedStringsState(false);

  // We're there motor off
  motorOff();
  setFailLedState(allOkay); // Show fault LED to indic tmo.
}



boolean openPanel(unsigned long ledFaderStart)
{
  /*
   This function opens the panel. Sets a start time ramps the
   motor then waits for the limit switch to be hit, 
   or a timeout to occur, whichever is the sooner.
   */
  unsigned long startTime;
  boolean allOkay = false;

  startTime = millis();
  rampPanelMotorUp(MOTOR_DRIVE_B);
  /* 
   Now, motor running, wait for the panel for timeout or
   limit switch to be hit Fade UP LEDS while we wait.
   */
  do
  {
    if (millis() > ledFaderStart)
    {  
      progressLedPwm(true); // This stops when it gets to zero.
    }
    delay(1);
    updateLimitSwStates();
    if (limitSwStates[0]== LOW) 
    {
      motorOff();
      Serial.println("Panel Fully Open Sensor hit");
      allOkay = true; // flag all okay.
      break; // Break out do-while. We're done
    }
    // Not there yet.
  }
  while ((millis() - startTime) < MAX_PANEL_MOVE_TIME);

  // One way or another we're there - ensure motor off
  setLedStringsState(true);
  motorOff();
  setFailLedState(allOkay);
}




void rampPanelMotorUp(int pwmLead)
{
  /*
This function ramps up the power to the panel drive motor using PWM.
   This helps to reduce the surge current into the motor and this offest
   power supply issues. See book text for troubleshooting tips in this
   area. 
   
   */

  setMotorControls(pwmLead); // Set the motor polarity

  // Now, ramp up the PWM slowly at first and the faster
  for (int lp=0;lp <= 255;lp++)
  {
    if (lp<=10) // Increase this number (max 30) to offset surge problems.
    {
      delay(15);
    }
    else
    {
      delay(3);
    }
    analogWrite(pwmLead,lp);
  }
}      


void motorOff()
{
  /* 
   This function just shuts the motor off 
   */
  analogWrite(MOTOR_DRIVE_A,0);  
  analogWrite(MOTOR_DRIVE_B,0);  
  digitalWrite(MOTOR_ENABLE, LOW); // Turn off the motor dvr chip
}   



void progressLedPwm(boolean fadingUp)
{
  /* 
   This function progresses the LED fading. The fadingUp boolean
   indicates which direction the fade is going. If the limit has
   been reached (fade down has reached zero, or fade up has 
   got to 255) then nothing happens. 
   
   In order to make sure that fading is pretty and doesn't happen
   too fast we use a ledDelayCtr. This effectively counts how many
   time this function is called and only does a fade step every
   LED_DELAY calls.
   */
  static int ledsPwmVal = 0;
  static int ledDelayCtr =0;
  
  if (ledDelayCtr <=0) // Does the call-counter need recharging?
  {
    ledDelayCtr = LED_DELAY; // Yes it does
  }
  if (--ledDelayCtr !=0) //Are we going to do something this time?
  {
    return; // No - go back.
  }
  // Okay, we're going to do a fade step. Decide if UP or DOWN.
  if (fadingUp == true)
  {
    if (ledsPwmVal < 255)
    {
      analogWrite(LED_STRINGS,++ledsPwmVal); // Increment & write 
    }
  }
  else
  {
    if (ledsPwmVal > 0)
    {
      analogWrite(LED_STRINGS,--ledsPwmVal); //Decrement & write
    }
  }
}


void setFailLedState(boolean theState)
{
  if(theState == true)
  {
    digitalWrite(FAIL_LED,HIGH); // Show fault light.
    Serial.println("Panel Transit Timeout!");
  }
  else
  {
    digitalWrite(FAIL_LED,LOW); // Ensure no fault light.
  }
}

void setLedStringsState(boolean allOn)
{
  if (allOn == true) 
  {
    analogWrite(LED_STRINGS,255);
  }
  else
  {
    analogWrite(LED_STRINGS,0);
  }
}


boolean returnSwitchState()
{
  /*
   A function to process the user button and return its state.
   Needs to be called often to be responsive to user input.
   */
  if (digitalRead(USER_SWITCH) == LOW)
  {
    Serial.println("Button pressed awaiting release");
  }
  else
  {
    // No button press just go back
    return(false);
  }

  /* 
   If we're still here then we need to wait for button release.
   else a user could hold the button down and make the
   panel cycle back and forth which it is not meant to do!
   This way, we force a press & release before we visibly react.
   */
  do
  {
    delay(10);
  }
  while (digitalRead(USER_SWITCH) == LOW) ;
  // Hey, hey! They released it. Do a debounce delay and get out.
  delay(30);
  return(true);  

}

void   updateLimitSwStates()
{
  // This function updates the limit switch array states from the hardware

  // Have these changed since last time?
  if (digitalRead(PANEL_FULLY_OPEN) != limitSwStates[0])
  {
    limitSwStates[0] = digitalRead(PANEL_FULLY_OPEN);
    Serial.println("Panel Fully Open.");
  }

  if (digitalRead(PANEL_FULLY_CLOSED) != limitSwStates[1])
  {
    limitSwStates[1] = digitalRead(PANEL_FULLY_CLOSED);
    Serial.println("Panel Fully closed.");
  }
}


void setMotorControls(int pwmLead)
{
  /* 
   A function that sets up the three control lines to the L293D motor
   controller. See book text for explanation of these. the pwmLead argument
   indicates which lead (Arduino pin number) is to be positive.
   */
  digitalWrite(MOTOR_ENABLE, LOW); // Disable the motor dvr chip
  // Set the motor lead that we are NOT going to PWM to low.
  if (pwmLead == MOTOR_DRIVE_A)
  {
    digitalWrite(MOTOR_DRIVE_B,LOW);
  } 
  else
  {
    digitalWrite(MOTOR_DRIVE_A,LOW);
  }
  digitalWrite(MOTOR_ENABLE, HIGH); // Enable the motor dvr chip
}

