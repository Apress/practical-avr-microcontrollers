/*

 Servo Motor Reset Program:
 Reset all listed servo motors to zero degrees
 */

#include <Servo.h>

#define HORIZ01_PIN 6
#define HORIZ02_PIN 9 
#define VERT01_PIN 10

Servo horiz01;
Servo horiz02;
Servo vert01;

void setup()
{

  Serial.begin(9600);
  Serial.println("Resetting motors");
  horiz01.attach(HORIZ01_PIN );
  horiz02.attach(HORIZ02_PIN );
  vert01.attach(VERT01_PIN);

  horiz01.write(0);
  horiz02.write(0);
  vert01.write(0);
  Serial.println("Reset done");

}

void loop()
{
  // Main loop left intentionally empty!
}



