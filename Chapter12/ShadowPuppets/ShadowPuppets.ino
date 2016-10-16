/*

A simple sketch to control the simple shadow puppet rig 
detailed in chapter 16 of "Practical AVR Microcontrollers".
The program runs in one of four modes:

00 = LED off, Motor Off
01 = LED on, Motor Off
02 = LED off, Motor On
03 = LED On, Motor On

When enabled, the motor sweeps back and forth to draw the
puppet across the light shone by the LED. This casts a moving
shadow on whatever surface the light hits (opposite wall etc). 
*/

#define SWITCH_PIN 8
#define MOTOR_PIN 9
#define LED_PIN 10

#include <Servo.h> 

Servo myMotor; // The servo motor
int theAngle=0; // Remembered angle of the motor. 
int thisMode =0; // Program's current mode.

void setup()
{
  myMotor.attach(MOTOR_PIN); // Setup the motor control pin
  pinMode(LED_PIN,OUTPUT); // Make the LED control pin an o/p
  pinMode(SWITCH_PIN,INPUT); // Make the switch pin an input.
  digitalWrite(SWITCH_PIN,HIGH); // Enable the pull up.

  digitalWrite(LED_PIN,LOW); // Ensure the LED starts Off
  myMotor.write(0); // Ensure the motor starts at home pos.

  Serial.begin(9600); // Init serial - minimal use.
}

void loop()
{
  // Is the switch pressed?
  if (digitalRead(SWITCH_PIN) == LOW)
  {
  do
    {
      delay(5); // Wait for switch to be released
    }
    while (digitalRead(SWITCH_PIN) == LOW);
    
    // Switch was released. 
    delay (5); // debounce delay
    thisMode++; // Increment the program mode
    if (thisMode >= 4) // If it maxes out, reset to zero
    {
      thisMode =0;
    }
    Serial.print("Software Mode Now = "); // Say the mode
    Serial.println(thisMode,DEC);
  }

// If bit zero of thisMode is set, then the LED is on.
  if (thisMode & 1)
  {
    digitalWrite(LED_PIN,HIGH);
  }
  else
  {
    digitalWrite(LED_PIN,LOW);
  }

// If bit one of thisMode is set, then advance the motor angle.
  if (thisMode & 2)
  {
    theAngle+=3;
    if (theAngle >=150) // Are we at end of travel for mtr?
    {
      theAngle = 0;  // We are, reset the motor back to home pos
    }
    myMotor.write(theAngle); 
  }

  delay(75);
  if (theAngle==0) // If we just reset the motor, give it time...
  {
    delay(1500);
  }

}
