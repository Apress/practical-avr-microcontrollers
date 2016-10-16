// ServoStepper1
#include <Servo.h> 

Servo myMotor;

void setup() 
{ 
  myMotor.attach(4);
} 

void loop() {
  for (int i=0;i<=180;i++) 
  {
    myMotor.write(i);  // Set angle to current value.
    delay(100);
  }

  myMotor.write(0); // Command back to home
  delay(1500);      // Wait for rewind.

} 


