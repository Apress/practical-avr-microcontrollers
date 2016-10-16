

int theCounter = 0;


void setup()
{
  Serial.begin(9600); // Initialise the serial channel pins.
}

void loop()
{
  Serial.print("The Counter now = "); // Msg without a new line
  Serial.println(theCounter);         // Msg with a new line.
  theCounter++;
  delay(1000);     // Go to sleep for one second (1000 milliseconds)
}

