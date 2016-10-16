#define LED1 8

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

