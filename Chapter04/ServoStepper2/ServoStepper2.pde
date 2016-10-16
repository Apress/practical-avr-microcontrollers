// ServoStepper2
#include <Servo.h> 

const int fullCCW =550;
const int fullCW = 2250;

Servo myMotor;

void setup() 
{ 
  myMotor.attach(4);
} 

void loop() {
  for (int i=fullCCW;i< fullCW;i+=10) 
  {
    myMotor.writeMicroseconds(i);  // Set angle to current value.
    delay(100);
  }

  myMotor.writeMicroseconds(fullCCW); // Command back to home
  delay(1500);      // Wait for rewind.

} 


