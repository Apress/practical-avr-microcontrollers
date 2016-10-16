/*

 CrazyBeams Version 3.
 
 There are (up to) five hardware entities involved
 3 (optionally, 2) servo motors.
 2 (optionally, 1) used for horizontal beam movement
 1 used for vertical beam movement. 
 2 (optionally 1) laser diodes which can be turned on or off

 Each motor has an at rest position, a start position 
 (which may be the same or different to its "at rest" position),
an end position (which is the highest angle this program will
ever set it to). and a maxPosition. This is the absolute max the
motor can do.
 */
#include <Servo.h>

#define VERS "Crazy Beams! Version 3.1c June 2012."
#define HORIZ01_PIN 6
#define HORIZ02_PIN 10
#define VERT01_PIN 9
#define LASER1_PIN 7
#define LASER2_PIN 8

Servo horiz01;
Servo horiz02;
Servo vert01;

struct servoMotorDetails {
  int motorPin; // Arduino pin number
  int restAngle; // Usually zero
  int startAngle; // Minium angle required
  int endAngle; // The highest useful angle
  int maxAngle; // Maximum angle allowed
  int currentAngle; // The current angle
  boolean movingForward; // Set to true if motor going fwd
  Servo servoRef;
};

// The following loads the default and initial motor settings. 
servoMotorDetails motors[] = {
  HORIZ01_PIN,0,10,90,180,0,true,horiz01,  
  HORIZ02_PIN,0,10,90,180,0,true,horiz02, 
  VERT01_PIN,0,40,130,180,0,true,vert01 };

const int mtrsArrayTop = 2; // Highest array member

// Some values that all functions use.
int theRand =0;
int loopDelay = 10;

// Defines the main loop speed min and max
const int LOOP_DELAY_MIN = 15;
const int LOOP_DELAY_MAX = 50;

boolean doNothing = false; // Set by the "S" command from Serial
boolean selfDisabled = false; // Set by the program for short time

void setup()
{
  // Seed random # generator from random noise on the A0 pin.
  randomSeed(analogRead(A0));

  // Attach the motors
  horiz01.attach(motors[0].motorPin);
  horiz02.attach(motors[1].motorPin);
  vert01.attach(motors[2].motorPin);

  // Flex the motors. All go to end, then back to rest.
  motorsRest();
  motorsToEnd();
  delay(750);
  motorsRest();
  delay(500);
  motorsToStart();

  // Setup the laser pins
  pinMode(LASER1_PIN,OUTPUT);
  pinMode(LASER2_PIN,OUTPUT);

  // Flash the lasers to show they work.
  for (int p=0;p<4;p++)
  {
    delay(150);
    lasersOn();
    delay(150);
    lasersOff();
  }
  Serial.begin(9600);
  Serial.println(VERS); // Init is done.
}

void loop()
{
  // See if there's any kb input to action
  if (Serial.available() !=0)
  {
    processKbInput(); // Yes there is. Go do it.
  }
  if (doNothing==true)
  {
    return; // Dont do anything if that's instructed.
  }

  /*
    Normally, down below here, we just progress the beams in
    a nice steady progress back and forth and up and down.
    However, the random number and switch table below 
    inject some randomness into this. 
    Change second number in the random line to larger value to
    make laser motion less eratic, or make it less (min 10) to 
    make it more eratic.
    */
  theRand = random(0,200);

  // Most random numbers do nothing. These are the exception.
  switch (theRand)
  {
    case 0:
      if (selfDisabled==false)
      {
        horizontalMotorRandom();
      }
      break;
      
    case 1:
      if (selfDisabled==false)
      {
        verticalMotorRandom();
      }
      break;
      
    case 2:
      selfDisabled= true;
      lasersOff();
      delay(1000);
      break;
      
    case 5:
      selfDisabled = false;
      break;

    case 6:
      changeLoopDelay();
      break;
  
    default:
      break;
  }    
          
  if (selfDisabled == true)
  {
    return; // Exit this function if nothing to do.
  }
  lasersOn();
  progressHorizMotion();
  progressVerticalMotion();
  delay(loopDelay);
}

// *** Various Utility Functions ***

void motorsHome()
{
  // This function makes all the servo motors seek their home position.
  for (int lp=0;lp<= mtrsArrayTop; lp++)
  {
    motors[lp].currentAngle =motors[lp].startAngle;
  }
  setMotorAngles(-1);
  delay(300); // Wait for the homes to happen. 
}  

void motorsRest()
{
  // All servo motors seek rest position.
  for (int lp=0;lp<= mtrsArrayTop; lp++)
  {
    motors[lp].currentAngle =motors[lp].restAngle;
  }
  setMotorAngles(-1);
  delay(300); // Wait for it to happen. 
}

void motorsToMax()
{
  // All servo motors seek max position.
  for (int lp=0;lp<= mtrsArrayTop; lp++)
  {
    motors[lp].currentAngle= motors[lp].maxAngle;
  }
  setMotorAngles(-1);
  delay(300); // Wait for it to happen. 
}


void motorsToEnd()
{
  // All servo motors seek max position.
  for (int lp=0;lp<= mtrsArrayTop; lp++)
  {
    motors[lp].currentAngle= motors[lp].endAngle;
  }
  setMotorAngles(-1);
  delay(300); // Wait for it to happen. 
}


void motorsToStart()
{
  // All servo motors seek start position.
  for (int lp=0;lp<= mtrsArrayTop; lp++)
  {
    motors[lp].currentAngle= motors[lp].startAngle;
  }
  setMotorAngles(-1);
  delay(300); // Wait for it to happen. 
}

void turnOnLasers()
{
  digitalWrite(LASER1_PIN,HIGH);  
  digitalWrite(LASER2_PIN,HIGH);  
}

void turnOffLasers()
{
  digitalWrite(LASER1_PIN,LOW);  
  digitalWrite(LASER2_PIN,LOW);  
}

void progressHorizMotion()
{
  /*
  This function progresses the motion of the horizontal motors 
   (in the array these are assumed to be [0][ and [1]) 
   by manipulating their in-memory descriptor as follows:
   
   If the movingForward member is true then:
   The currentAngle member of the array associated with each motor
   is incremented. If it becomes more than maxAngle then it is
   decremented again, and movingForward for this motor is set
   to false so that next time it will be decremented.
   
   If the movingFoward member is false then:
   the currentAngle member of the array is decremented and if that
   makes it less than startAngle, then movingForward is set to true.
   */

  for (int ctr=0;ctr<=1;ctr++)
  {
    if (motors[ctr].movingForward == true)
    {
      // Moving fwd
      motors[ctr].currentAngle++;
      if (motors[ctr].currentAngle >= motors[ctr].endAngle)
      {
        motors[ctr].currentAngle--;
        motors[ctr].movingForward = false;
      }
    }
    else
    {
      // Moving backward
      motors[ctr].currentAngle--;
      if (motors[ctr].currentAngle <= motors[ctr].startAngle)
      {
        motors[ctr].currentAngle++;
        motors[ctr].movingForward = true;
      }
    }
    /* 
     At this stage all we have done is change the in-memory
     settings for this motor. Now, we make it real. Write it
     out to the hardware
     */
    setMotorAngles(-1);
  }
}

void progressVerticalMotion()
{
  /*
  This function progresses the motion of the vertical motor by
   manipulating its in-memory descriptor as follows:
   
   If the movingForward member is true then:
   The currentAngle member of the array associated with each motor
   is incremented. If it becomes more than maxAngle then it is
   decremented again, and movingForward for this motor is set
   to false so that next time it will be decremented.
   
   If the movingFoward member is false then:
   the currentAngle member of the array is decremented and if that
   makes it less than startAngle then movingForward is set to true.
   */

  if (motors[2].movingForward == true)
  {
    // Moving fwd
    motors[2].currentAngle++;
    if (motors[2].currentAngle >= motors[2].endAngle)
    {
      motors[2].currentAngle--;
      motors[2].movingForward = false;
    }
  }
  else
  {
    // Moving backward
    motors[2].currentAngle--;
    if (motors[2].currentAngle <= motors[2].startAngle)
    {
      motors[2].currentAngle++;
      motors[2].movingForward = true;
    }
  }
  /* 
   At this stage all we have done is change the in-memory
   settings for this motor. Now, we make it real. Write it
   out to the hardware
   */
  setMotorAngles(-1);

}

void horizontalMotorRandom()
{
  // Set a random position for each of the horizontal motors.
  for (int lp=0;lp<=1;lp++)
  {
    motors[lp].currentAngle =
      random(motors[lp].startAngle,
    motors[lp].endAngle+1); // Random betwen min and max 
  }
  setMotorAngles(-1);
}

void verticalMotorRandom()
{
  // Set a random position for the vertical motor.
  motors[2].currentAngle =
    random(motors[2].startAngle,
  motors[2].endAngle+1); // Random betwen min and max 

  setMotorAngles(2);

}

void setMotorAngles(int motorID)
{
  /*
  This function sets motor hardware to reflect the in-memory
   currentAngle settings. If motorID is -1 it writes to all the
   motors in the array. This is hard coded cos there's no way to
   dynamically discover the size of the array at run-time 8-(
   */
  if (motorID == -1)
  {
    motors[0].servoRef.write(motors[0].currentAngle);
    motors[1].servoRef.write(motors[1].currentAngle);  
    motors[2].servoRef.write(motors[2].currentAngle);  
  }
  else
  {
    motors[motorID].servoRef.write(motors[motorID].currentAngle);  
  }  
}

void lasersOn()
{
  digitalWrite(LASER1_PIN,HIGH);
  digitalWrite(LASER2_PIN,HIGH);
}

void lasersOff()
{
  digitalWrite(LASER1_PIN,LOW);
  digitalWrite(LASER2_PIN,LOW);
}


void processKbInput()
{
  byte theChar;
  theChar=Serial.read();
  switch (theChar)
  {
  case 's':
  case 'S':
    {
      doNothing = !doNothing;
    }
    if (doNothing)
    {
      lasersOff();
    }
    break;

  case 'l':
  case 'L':
    if (digitalRead(LASER2_PIN) ==HIGH)
    {
      lasersOff();
    }
    else
    {
      lasersOn();
    }
    break;

  case '1':
    digitalWrite(LASER1_PIN,HIGH);
    digitalWrite(LASER2_PIN,LOW);      
    break;

  case '2':
    digitalWrite(LASER1_PIN,LOW);
    digitalWrite(LASER2_PIN,HIGH);      
    break;
    
  default:
    Serial.print("Unknown Command: ");
    Serial.println(theChar);
    Serial.println("Commands: 'S', 'L', '1', '2'");
    break;
  }
}


void changeLoopDelay()
{
  // This sets the loopDelay (on which the speed of the main loop
  // relies) to a random number within the required range. 
  loopDelay=random(LOOP_DELAY_MIN,LOOP_DELAY_MAX);
}
