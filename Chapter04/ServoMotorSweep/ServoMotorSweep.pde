/*
  Servo Motor Sweep. Makes a servo motor sweep from one end of
 its travel to the other. 
 */

int motorPin = 4;                 // Name the motorPin

/*
  Constant values used through the program
 */

/* 
 Pulse length (in microseconds) for fully counter clockwise
 and fully clockwise. Customize to your servo motor's specs.
 */
const int fullCCW =750;
const int fullCW = 2250;

const int frameRate = 50; // How many servo frames/second
const int msPerFrame = 1000 /frameRate; // Millisecs per frame

// Do the setup
void setup()
{
  pinMode(motorPin, OUTPUT);      // Make the pin an output
  Serial.begin(9600);
  Serial.print("msPerFrame=");    // Say the frame length val.
  Serial.println(msPerFrame,DEC);
}

// Now loop round this forever and ever. 
void loop()
{
  /*
    We start from the fully counter clockwise value, then each 
   time through the main program loop we lengthen the pulse
   by 5 microseconds. Then, we pause for enough time to allow
   the rest of the 50ms frame time to pass. This is not going
   to be 100% accurate due to execution delays and latency, but
   it's easily accurate enough (about 98% accurate) for the servo 
   motor - which in any case varies from example to example.
   To make this 100% accurate we would need to hook up an
   oscilloscope or a digital analyzer to the motorPin and tweak 
   the program values until we had it right, however that would
   be overkill for this purpose.
   */
  for (unsigned int myDelay = fullCCW; myDelay < fullCW; myDelay++)
  {
    // Frame start
    digitalWrite(motorPin, HIGH);   // The pulse starts.
    delayMicroseconds(myDelay);     // pause for current pulse microseconds      
    digitalWrite(motorPin, LOW);    // The pulse ends.
    /*
     Now wait out the rest of the frame time which is our 
     frame time (in milliseconds) minus the pulse length in 
     milliseconds (1000 milliseconds to a microsecond)
     */
    delay(msPerFrame-(myDelay/1000));        
  }
}

