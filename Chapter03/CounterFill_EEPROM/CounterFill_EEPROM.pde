#include <EEPROM.h>;

void setup()
{
/* 
   This runs once at startup. It writes each of the first 
   255 EEPROM byte's address into itself, if it has not 
   already been done. */
for (int thisAddr = 0; thisAddr <=255; thisAddr ++)
  {
    if (EEPROM.read(thisAddr) != thisAddr) 
    { 
      EEPROM.write(thisAddr,thisAddr);
    }
  } 
}

void loop()
{
// Do nothing.....
}
  
