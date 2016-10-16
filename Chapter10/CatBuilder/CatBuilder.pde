#include <stdio.h>
/*
catBuilder: Version 1.0b:
 
 AVR chips (at least the older ones) soon run out of RAM memory if you have lots of
 strings such as user messages or help texts in your application. The ideal solution
 to this problem is to store messages in a message catalogue held in a non-volatile
 area of memory. Then, whenever the software needs to output any kind of message
 it simply looks up the message in the catalogue and spits it out to the interface
 being used (serial channel, LCD display or whatever). This takes a little more
 effort to do, but it saves copious amounts of RAM memory. 
 
 The question then arises of where to find this EEPROM memory for the message
 catalogue. There are only two possibilities on an AVR chip. 
 
 First: The catalogue could be stored in an unused part of the program EEPROM space.
 This idea is initially attractive because the EEPROM space on most AVRs is seldom
 100% used by software. Further, the Arduino/WinAVR development environment provides
 the PROGMEM keyword which, when prepended to a variable declaration, instructs 
 the compiler to put the variable into program memory. However, there is a problem
 which is that retrieving such variables from program memory at run-time is a bit
 clunky and the code reads like a nightmare because you have know at what address
 your variable was placed. After playing with this possibility for a while I have
 discounted this method as being too non-intuitive and messy for implementing
 a message catalogue.
 
 Second possibility: data EEPROM. All ATMEGA AVR chips have an area of EEPROM
 that is intended for applications to store data which needs to be non-volatile
 into. Again, few applications use very much of the provided capacity of this
 EEPROM area. Attractively, the use of this area of memory is easy. Using the 
 provided EEPROM.h library, the EEPROM is accessed using just simple reads and 
 writes. Therefore, this is the one I have adopted in this code for building a 
 message catalogue. 
 
 This message catalogue builder needs to be separate to the application which
 uses the catalogue because, if the code to write the EEPROM were included in
 the main application, the RAM usage at run time would not be reduced. At the
 time of writing, there is an unsuported EEMEM keyword that would allow the
 compile-time specification an area of EEPROM memory, but again this is complex
 to use and - since it is unsupported - liable to change. 
 
 The scheme therefore is to build the message cataogue with this software
 but to retrieve it with other programs. Whilst this CatBuilder application
 has been created to create a catalogue for the WordDune application it can
 readily be adapted to future uses, in tandem with the returnCatMsg function
 used by WordDune. 
 
 Alan T: 18/09/2011
 
 */


#include <EEPROM.h>

const String applicationName = "WordDune";

// The message catalogue is a damn great array of char.
char* messageCat[] = {
  "\n\r\nWord Dune: Version ",                     // Message #0
  "Base dictionary rebuilt",                 // Msg #1
  "=======",                                 // Msg #2
  " entries found.",                         // Msg #3
  " Free space = ",                          // Msg #4
  "Word was added",                          // Msg #5
  "Word NOT added: ",                        // Msg #6 
  "Insufficient space.",                     // Msg #7
  "Dictionary corrupt?",                     // Msg #8
  "Minimum char count is ",                  // Msg #9
  "Not added: Error code = ",                // Msg #10
  "Unknown cmd.",                            // Msg #11
  "No such word!",                           // Msg #12
  "Word exists!",                            // Msg #13
  ""                                         // An empty string ends the table
};


char* helpText = {
  "Commands:\n\r"             
    "ADD [Word]: Add new word\n\r"    
    "DEP: Dump EEPROM\n\r"
    "DIC: List dictionary\n\r"
    "LEV: set/show game level\n\r"
    "DEL: Delete word\n\r"
    "SWC: Show word count\n\r"};

const int EEPROM_SIZE = 1024;

void setup (){

  Serial.begin(9600);
  Serial.println("Message Catalogue Builder. V1");
  format_EEPROM(EEPROM_SIZE); // This size should NOT be hard coded.

  /* The EEPROM format is as follows:
   
   Bytes 0&1 = EEPROM address of the message catalogue (low byte first).
   Bytes 2&3 = EEPROM address of the help message catalogue
   Bytes 4&5 = EEPROM address of the start of the dictionary
   
   The message catalogue, the help text and the dictionary are all simply sets of 
   in-memory strings in which the last byte has got bit 7 set. A byte containing 255
   indicates the end of a catalogue/dictionary.
   
   The following functions load these structures based on the declarations above. 
   For the purposes of common coding, the message catalogue is referred to as cat#0, 
   the help as cat#1 and the dictionary as cat#2
   */

  word ePtr=6; // This is our pointer. It starts from byte 6
  byte myChar =0;

  //load the message cat.
  int arrayPtr=0;
  int i = 0;
  while (strlen(messageCat[arrayPtr]) != 0) {
    for (i=0;i<strlen(messageCat[arrayPtr]);i++){
      // If this is the last byte of the msg, write it with bit 7 set.
      if (i < (strlen(messageCat[arrayPtr])-1)){
        EEPROM.write(ePtr++, messageCat[arrayPtr][i]); // Not the last
      }
      else
      {
        EEPROM.write(ePtr++,messageCat[arrayPtr][i] + 128); // Write last with bit 7 set. 
      }
    }
    arrayPtr++;
  }
  // Now, the current value of ePtr is the start of the next cat
  // and we know our one started at 0006. Save these 
  EEPROM.write(0,6);
  EEPROM.write(1,0);
  EEPROM.write(2,ePtr & 255);
  EEPROM.write(3,ePtr>>8);

  Serial.println("Main message cat saved.");

  // Now do the help text, but only if it would not overflow the available EEPROM. 
  int x = (ePtr + strlen(helpText));
  if (x < EEPROM_SIZE) {

    for (i=0;i<strlen(helpText);i++){
      if (i < (strlen(helpText)-1)) {
        EEPROM.write(ePtr++,helpText[i]);
      }
      else
      {
        EEPROM.write(ePtr++,helpText[i]+128);
      }
    }
    Serial.println("Help text programmed.");
  }
  else
  {
    Serial.println("SETUP ERROR: Help Text overflows EEPROM!"); 
    Serial.print((x- EEPROM_SIZE),DEC); 
    Serial.println(" additional bytes needed. Can't proceed.");
  }
  // Program the starting vector for the dictionary
  EEPROM.write(4,ePtr & 255);
  EEPROM.write(5,ePtr>>8);
  dumpEEPROMToSerialChannel();
  Serial.print("Bytes of EEPROM remaining = ");
  Serial.println(EEPROM_SIZE - ePtr,DEC);
}

void loop() {
  // That's it, there is  no loop!
}


void format_EEPROM(int size) {
  // this function formats the EEPROM to contain all FFs 
  // - if it doesn't already.
  byte x = 0;
  int wroteCount = 0;

  Serial.print("Formatting EEPROM ");  
  Serial.print(size);  
  Serial.println(" bytes.");

  for (int L = 0; L <=size;L++) {
    x=EEPROM.read(L);

    if (x != 255) {
      EEPROM.write(L,255);
      wroteCount++;
    }
  }
  Serial.print("Done Formating: ");  
  Serial.print(wroteCount,DEC);  
  Serial.println(" bytes needed erasing.");
}


void dumpEEPROMToSerialChannel()
{
  byte ourChar;
  char ascChars[17];
  for (int c=0;c <EEPROM_SIZE;c+=16){
    // Do leading zeroes as required to keep column format
    if (c < 16){
      Serial.print("00");
    }
    else
    {
      if (c<255) {
        Serial.print("0");
      }
    }
    // Print the hex address 
    Serial.print(c,HEX); 
    Serial.print(": ");    
    ascChars[0]=0; //init the character buffer

    for(int d=0;d<16;d++){
      ourChar= EEPROM.read(c+d); //get char
      if (ourChar <16) { // Again add leading zero if reqd
        Serial.print("0");
      }
      Serial.print(ourChar,HEX);
      Serial.print(" ");

      if (! isalnum(ourChar)){ // add this one to char buffer
        ourChar=' ';
      }
      ascChars[d] = ourChar;
      ascChars[d+1]=0;
    }
    Serial.print("  "); 
    Serial.println
      (ascChars);
  }
  Serial.println(); // Make sure we set the cursor to a new line when we exit.
}


byte writeByte(int address, byte value) {
  /*
  Some loverly code to write bytes to the EEPROM, with a check to make sure the
   address is within range of the available memory.
   Returns:
   0 = Byte was written and read back okay.
   1 = Byte was written, but failed to read back correctly.
   -1 = Address exceeds available EEPROM space
   -2 = Address was invalid (negative). 
   */
  if (address <0) {
    return(-2);
  }

  if (address > EEPROM_SIZE) {
    return(-1);
  }

  // Go for it.
  EEPROM.write(address,value);
  if (EEPROM.read(address) != value){
    return(1);
  }
  return(0);
}


