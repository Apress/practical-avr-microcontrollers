
const int SAMPLES_PER_PASS = 10;

void setup()
{
  Serial.begin(9600);
}

void loop()
{
  int avg=0;
  for (int i=0;i<=SAMPLES_PER_PASS;i++) 
  {
    delay(50);
    avg += analogRead(A5);

  }
  Serial.println(avg/SAMPLES_PER_PASS,DEC);
  avg=0;
  delay(150);
}


