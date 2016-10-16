// This program allows you to step the motor manually by
// sending keyboard commands "u", "d", "z" and "h"
// meaning up, down, zero and halfway respectively.
#include <Servo.h> 

Servo myMotor;
int theAngle=0;

void setup() 
{ 
  Serial.begin(9600);
  Serial.println("Servo Calibrator V1");
  myMotor.attach(4);
} 

void loop() {
  if (Serial.available() !=0)
  {
    // Something has been received.
    int inChar = Serial.read();
    switch (inChar) 
    {
    case 'u':
      theAngle++;
      break;
      
    case 'd':
      theAngle--;
      break;
      
    case 'h':
      theAngle=90;
      break;
      
    case 'z':
      theAngle = 0;
      break;
      
    default: // Default case, say what...
      Serial.println("use u, d, h or z only");
      break;
    } 
    if (theAngle < 0) 
      theAngle=0;
    if (theAngle > 180)
      theAngle=180;
    Serial.print("Angle is now set to ");
    Serial.println(theAngle,DEC);
  }
  myMotor.write(theAngle);
} 




