#define LED1 8
Static String myString = "If you can read this you are looking into the AVR flash memory";
void setup()
{
  pinMode(LED1, OUTPUT);
}

void loop()
{

  if (digitalRead(LED1)==LOW) 
  {
    digitalWrite(LED1, HIGH);
  }
  else
  {
    digitalWrite(LED1, LOW); 
  }
  delay(4000);
}

