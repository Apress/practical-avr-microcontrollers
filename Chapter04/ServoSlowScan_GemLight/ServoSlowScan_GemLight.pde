// ServoSlowScan_GemLight
// This program sets a servo motor to slow scan so that
// when a Luxeon LED is lashed to it, off-centre, and 
// the whole assembly is placed under a transparent crystal
// holder filled with glass crystals they will scintillate!
// You can stop the scan by sending sending keyboard command
// "s" or start it by sending "g" 

#include <Servo.h> 

Servo myMotor;
int theAngle=0;
boolean countingUp = true;
boolean doMovement = true;

void setup() 
{ 
  Serial.begin(9600);
  Serial.println("Servo Slow Scan for GemLight");
  myMotor.attach(4);
} 

void loop() {
  delay(75);
  if (doMovement == true){
    if (countingUp == true) 
    {
      theAngle++;
      if (theAngle >=180) 
      {
        countingUp = false;
      }
    }
    else
    {
      theAngle--;
      if (theAngle <=0)
      {
        countingUp = true;
      }
    }
    myMotor.write(theAngle);    
  }

  if (Serial.available() !=0)
  {
    // Something has been received.
    int inChar = Serial.read();
    switch (inChar) 
    {
    case 's':
      doMovement = false;
      Serial.println("Gemlight Stopped.");
      break;

    case 'g':
      doMovement = true;
      Serial.println("Gemlight Started.");
      break;

    default: // Default case, tell them what we need.
      Serial.println("use s for 'stop' or g for 'go'");
      break;
    } 

  }


} 






