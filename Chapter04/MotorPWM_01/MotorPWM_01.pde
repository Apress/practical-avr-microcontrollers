/*
 This program uses PWM to control a motor via a driver
 transistor. It cycles through all possible PWM values (0 to 255)
 at a rate of one speed step every MS_BETWEEN_STEPS milliseconds.
 It will recognize and action a few single-letter commands sent 
 via the serial channel. (In these descriptions, SS means Speed 
 Stepping)
 
 "a" toggles SS on/off - initial state is ON
 "d" decrements the current pwm value.
 "f" sets pwm value to 255 (full on) & disables SS
 "u" increments the current pwm value.
 "z" sets pwm value to zero (stops motor) & disables SS
 */

const int MOTOR_PIN = 9; // Must use a PWM capable pin for this.
const int MS_BETWEEN_STEPS = 500; // Millisecs between speed steps
byte pwmVal = 0;
byte lastPwm = 0;
int msCounter =0;
boolean incPwmEnabled = true;

void setup() 
{ 
  Serial.begin(9600);
  Serial.println("PWM tester V1.0d");
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












