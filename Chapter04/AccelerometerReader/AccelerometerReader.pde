/*
This sketch is to interrogate and continuously display the output values 
from an MM7260 Accelerometer. The X, Y and Z outputs are attached to 
analogue inputs 0,1 and 2 respectively. A 3.3V source is needed to supply 
AREF and to power the chip - since that is the supply voltage required. 
See book text for more details. */

#define X_IN 0
#define Y_IN 1
#define Z_IN 2

void setup(){
  // Not much to do here.
  Serial.begin(9600);
  analogReference(EXTERNAL);
}

void loop(){
  Serial.print(" Accelerometer Values: X = " );
  Serial.print (analogRead(X_IN),DEC);

  Serial.print(": Y = " );
  Serial.print (analogRead(Y_IN),DEC);

  Serial.print(": Z = " );
  Serial.println (analogRead(Z_IN),DEC);
  
  delay(100);
}

