/*
Motor PWM Tester V2
 This program uses PWM to control a DC motor via a driver
 transistor. It cycles through all possible PWM values (0 to 255)
 at a rate of one speed step every MS_BETWEEN_STEPS milliseconds.
 It will recognize and action a few single-letter commands sent 
 via the serial channel. (In these descriptions SS means Speed 
 Stepping)
 
 "a" toggles SS on/off - initial state is ON
 "d" decrements the current pwm value.
 "f" sets pwm value to 255 (full on) & disables SS
 "r" ramps from zero to full in 5 seconds & disables SS
 "R" ramps from zero to full in 1 second & disables SS
 "s" ramps full to zero in 5 seconds & disables SS
 "S" ramps full to zero in 1 second & disables SS
 "u" increments the current pwm value.
 "z" sets pwm value to zero (stops motor) & disables SS
 */

const int MOTOR_PIN = 9; // Must use a PWM capable pin for this.
const int MS_BETWEEN_STEPS = 500; // Millisecs between speed steps
const int MOTOR_SPEED_DELAY = 1000; // Motor 0 to full ramp time
byte pwmVal = 0;
byte lastPwm = 0;
int msCounter =0;
boolean incPwmEnabled = true;

void setup() 
{ 
  Serial.begin(9600);
  Serial.println("PWM tester V2.0d");
  pinMode(MOTOR_PIN,OUTPUT);
  analogWrite(MOTOR_PIN,pwmVal);
} 

void loop() 
{
  delay(1); // Wait a millisecond.
  if (++msCounter >= MS_BETWEEN_STEPS) 
  {
    msCounter = 0;
    if (incPwmEnabled == true)
    {
      pwmVal++;
    }
  }
  if (Serial.available() !=0)
  {
    // Something has been received.
    int inChar = Serial.read();
    switch (inChar) 
    {
    case 'a':
      Serial.print("Speed Stepping is now ");
      if (incPwmEnabled==true)
      {
        incPwmEnabled= false;
        Serial.println("OFF");        
      }
      else
      {  
        incPwmEnabled = true;
        Serial.println("ON");        
      }
      break;

    case 'f':
      pwmVal= 255;
      incPwmEnabled = false;
      break;

    case 'u':
      pwmVal++;
      break;

    case 'd':
      if (pwmVal != 0)
        pwmVal--;
      break;

    case 'r':
    case 'R':
      // Do 5 or 1 second ramp up
      incPwmEnabled = false;
      if (inChar == 'r') 
      {
        doRampUp(5000);
      }
      else
      {  
        doRampUp(1000);
      }
      pwmVal=255;      
      break;


    case 's':
    case 'S':
      // Do 1 or 5 second ramp down
      incPwmEnabled = false;
      if (inChar == 's') 
      {
        doRampDown(5000);
      }
      else
      {  
        doRampDown(1000);
      }
      pwmVal=0;
      break;

    case 'z':
      pwmVal = 0;
      incPwmEnabled = false;
      break;

    default: // Default case, say what...
      Serial.println("use u, f, d or z only");
      break;
    } 
  }
  // If the value has changed, check it, correct it and write it out
  if (lastPwm != pwmVal)
  {
    if ((pwmVal >=255)  && (incPwmEnabled == true))
    {
      pwmVal=0;
    }
    analogWrite(MOTOR_PIN,pwmVal);
    Serial.print("pwmVal is now ");
    Serial.println(pwmVal,DEC);
    lastPwm = pwmVal;
  }
} 


void doRampUp(int rampLenMs)
{
  /*
    Stops the motor. Waits MOTOR_SPEED_DELAY ms for motor 
   to actually stop, then ramps up the motor to full pwm 
   speed in the number of milliseconds required by the caller.
   */
  int msVal = int(rampLenMs/255);
Serial.println(msVal,DEC);

  analogWrite(MOTOR_PIN,0);
  delay(MOTOR_SPEED_DELAY);
  for (int myLoop =0; myLoop <=255; myLoop ++)
  {
    delay(msVal);
    analogWrite(MOTOR_PIN,myLoop);
  }  
}

void doRampDown(int rampLenMs)
{
  /*
   Ramps down the motor from full to zero pwm in the number of 
   milliseconds required by the caller.
   */
  int msVal = int(rampLenMs/255);

  analogWrite(MOTOR_PIN,255);
  delay(MOTOR_SPEED_DELAY); // Wait for full speed
  for (int myLoop =255; myLoop >=0; myLoop --)
  {
    delay(msVal);
    analogWrite(MOTOR_PIN,myLoop);
  }  
}










