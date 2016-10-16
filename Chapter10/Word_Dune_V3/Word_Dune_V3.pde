
/*
 Word Dune: A game in which actual words or phrases are semi-hidden in a torrent of random characters and
 letters. The torrent gets faster and denser at each level of the game. There are four words, each flashed
 through a number of times (varies by level). At the end of each level you have to know what the words
 were and check them against the revealed list.
 
 By default when run, will use the default word sets. However, the dictionary of words can be updated via
 the serial channel via a set of three letter commands. Use the HEL command to see all available commands
 New words can be added, old words removed. You can list the dictionary. You can also see the entire 
 contents of the EEPROM if you want to. By default, has a set of 10 words.
 
 For a new chip you should use AVR studio to clear the flag "EEPROM Erase" to prevent the EEPROM from 
 being erased at every reprogramming of the program memory. Then you should upload and run the catBuilder
 program. This sets up the EEPROM contents to an initial known state. You can then upload this code to
 make the game ready to use.
 
 Uses a liquid Crystal display (LCD) with:
 rs on digital pin 12
 rw on digital pin 11
 enable on digital pin 10
 d4-7 on digital pins 5-2
 
 the user's "click" button is on digital pin 8
 */
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <stdlib.h>

LiquidCrystal lcd(12, 11, 10, 5, 4, 3, 2);

unsigned long  myCounter = 0;

boolean DO_FORMAT = false; // If this is true then the EEPROM will be reformated when code runs. DEBUG use only.
const String prompt = "WordDune> ";
const String Version = "3.0b";

// LCD Params
const int rowCount = 4;
const int columnCount = 20;

// General consts
const int EEPROM_SIZE = 1024;
const int MIN_WORDLENGTH = 4;
const int MAX_WORD_LEN = 20;
const int MAX_MESSAGES = 100; // How many messages can be in a cat...?
const int WORDS_PER_LEVEL = 4; // How many duneWords per level?
const int MAX_LEVEL = 6;
const int DEBOUNCE_DELAY = 5; // ms delay when reading switch input.

// Current level variable
int currentLevel=0;

// Various char buffers used throughout.
char stringBuff[20];
char inputBuffer[20];
int inputBufferPtr=0;

// These are the default dictionary words. Change them if you wish.
char* firstWords[] = { 
  "SUPERIOR", "CLIFF", "INFLUENZA",  "PACIFIC", 
  "MADRIGAL", "POWERHOUSE", "CANADA", "MATRIX",
  "GENEROUS", "ALBATROSS"};

// Word descriptors.
struct wordDesc {
  int wordIndex;
  int tryCount;
};

wordDesc duneWords[WORDS_PER_LEVEL];

// Level Descriptors
struct levelDesc {
  int perCharDelay;       // How long between each random char appearing (millis)
  int perWordDelay;        // How long should we pause after putting a complete word on display
  int showWordCount;       // How many times should each word appear at this level
  int wordFrequency;       //How often should a word appear (as a percentage of chars put)
  boolean randomiseScreen; // Should screen start with crap all over it?
};

levelDesc levelData[7] = {
  75, 85, 6, 7, false,
  55, 80, 6, 7, false,
  50, 75, 5, 5, false, 
  47, 70, 4, 4, false,
  30, 65, 6, 4, true,
  20, 50, 8, 3, true,
  18, 45, 8, 4, true  }; 



// Here comes the setup code.

void setup()
{
  Serial.begin(9600);

  lcd.begin(columnCount, rowCount);
  lcd.clear();
  // Put out the program name and version to serial and LCD.
  sayMessage(0,0,true, false);
  Serial.println(Version);
  pinMode(8,INPUT);
  digitalWrite(8,HIGH);

  // If EEPROM doesn't contain anything yet, or we are forcing format, then format it now.
  if ((DO_FORMAT == true) || (EEPROM.read(returnDictionaryBase()) == 255)) {
    format_EEPROM(EEPROM_SIZE,returnDictionaryBase());
  }

  // Are there enough words to get started with?
  if (returnDictWordCount() < 4) {
    for (int myLoop=0;myLoop <=9;myLoop++){
      int w = addToDictionary(firstWords[myLoop]);
    }
    sayMessage(0,1,true, true); // Say it's been done
  }
  currentLevel=0; // We start from level 0
  Serial.print(prompt); // Command line prompt in case anyone is listening.
  randomSeed(analogRead(1) * analogRead(2));
  doSplashScreen();
  lcd.clear();
}

void loop()
{
  unsigned long lp=0;
  int thisInt=0;
  byte aChar=0;
  boolean levelComplete = false;

  // This is the start of the level loop
  if (selectFourWords() <0) {
    /*
    If there are fewer than four words when we come to select some, then
     someone has removed them during run time. We need to restart the unit
     to add the default words. The game does not work with fewer then four 
     words in the dictionary.
     */
    lcd.clear();
    sayThis("< than 4 words.",true,true,true);
    lcd.setCursor(0,1);
    sayThis("Please restart.",true,true,true);
    while(1); 
    {
    }
  }


  doLevelIntroducer(currentLevel,4);

  /*   
   Ensure we don't start a level until the click switch is released.
   We usually get here because they clicked so we don't want to proceed
   until they leggo the button.
   */

  do {
  } 
  while( digitalRead(8) == LOW);

  levelComplete=false;

  // Rendomise the screen if this level descriptor dictates it.
  if (levelData[currentLevel].randomiseScreen==true) {
    fillScreenWithRandomChars();
  }
  /* 
   Here beginneth the level loop.
   Write appropriately random chars according to the current level. 
   */

  while(levelComplete==false) {
    lcd.setCursor(random(0,columnCount),random(0,rowCount));

    switch (currentLevel) {

    case 0: 
      aChar= 32; // debug/training mode spaces as the dune! Childishly simple
      break;

    case 1:
      aChar = byte(random(33,47)); // Various symbols and punctuation marks as the dune: Very easy    
      break;

    case 2:
      aChar= byte(random(0x30,(0x30+9))); // Numbers as the dune - easy
      break;

    case 3:                            // Only lower case letters as dune. Tricky
      aChar = byte(random(97,123));
      break;

    default:      
      aChar = byte(random(65,(65+26))); // Only upper case letters as the dune - hardest.
      break;
    }
    lcd.print(aChar);

    // Now use a delay to limit the draw speed according to current level params.
    delay(levelData[currentLevel].perCharDelay);

    lp=random(1,101); // Generate random number from 1 to 100. Then, use that number in
    // conjunction with the current level's word frequency to see if it's time
    // to put another complete word on the screen.
    if (lp <levelData[currentLevel].wordFrequency) {
      /* 
       We already chose four words for this level, choose one of them at random.
       This returns the number of the selected word (or -1 if all words have been shown
       the required number of times) 
       */
      thisInt=chooseRandomWordIndexIndex(WORDS_PER_LEVEL -1);
      if (thisInt == -1) {
        // Looks like we have shown all the words. This level is done. Set the flag.
        levelComplete= true;
      }
      else { 
        // No, we can show this word. Get it and show it now at a random location.
        getDictWord(duneWords[thisInt].wordIndex,stringBuff);
        lcd.setCursor(random(0,(columnCount - strlen(stringBuff))),random(0,rowCount));
        lcd.print(stringBuff);
        delay(levelData[currentLevel].perWordDelay);
      }
    }
    /* 
     Now just before the next loop, lets see if the user wants to bale out of this level.
     Have the clicked the button?
     */
    if (digitalRead(8) == LOW)  {
      // They have. Set the flag.
      levelComplete=true;
    }
    doSerialInput(); // Do serial port input if there is any.
  }
  // Okay, now we are out of the game loop. Do the level end...
  doLevelEnd();
  lcd.clear();

  // Step to the next level, if there is one, and loop again, or say the game has ended.
  currentLevel++;
  if (currentLevel > MAX_LEVEL){
    lcd.clear();
    lcd.print("Game over!"); 
    lcd.setCursor(0,1);
    lcd.print("Click to play again!");
    awaitKeypress(20000,false,8);
    doSplashScreen();
    currentLevel=0;
  }
}

void format_EEPROM(int size, int startByte) {
  /*
  This function writes FFs in the EEPROM from startByte to the end. By making
   startByte a value midway through EEPROM you can format only the dictionary
   area, and leave the rest intact.
   */
  byte x = 0;
  for (int L = startByte; L <size;L++) {
    x=EEPROM.read(L);
    if (x != 255) {
      EEPROM.write(L,255);
    }
  }
}


int addToDictionary(char* theWord){
  /*
  Given a word, this function tries to add it to the EEPROM dictionary.
   It returns:
   0 if the word was added 
   -1 if there was no space
   -2 if the EEPROM seems to be corrupted
   -3 if the word supplied was invalid in some way (such as already exists).
   */
  if (strlen(theWord) <MIN_WORDLENGTH) {
    return(-3);
  }

  if (returnWordLocation(theWord) >0) {
    return(-4);
  }
  // Find the first 255 (unused byte) in the EEPROM
  for (int a=returnDictionaryBase();a<=EEPROM_SIZE;a++) {
    if (EEPROM.read(a) == 255) {
      // Okay this is it. Will this word fit? Write it to EEPROM if so.
      if ((a + strlen(theWord) > EEPROM_SIZE )){
        // No Space. abort...
        return (-1);
      }

      for (int p=0; p < strlen(theWord); p++) {
        if (p != (strlen(theWord))-1) {
          EEPROM.write(a++,theWord[p]);
        }
        else
        {
          byte thisChar = theWord[p];
          thisChar+=128;
          EEPROM.write(a++,thisChar);
          return(0);
        }
      }
    }
  }
  // If we ever get right through this loop we never found a
  // 255 char which means that the EEPROM is corrupted
  return(-2);
}


void dumpEEPROMToSerialChannel(){
  /*
    This function dumps the enture EEPROM contents to the serial channel. The 
   output provided is in the standard Hexdump format used by such tools the world
   over.
   */
  byte ourChar;
  char ascChars[17];

  for (int c=0;c <EEPROM_SIZE;c+=16){
    // Format and output the EEPROM address for this row
    sprintf(stringBuff,"%04x: ",c);
    Serial.print(stringBuff);

    // Now do the row of values.
    for(int d=0;d<16;d++){
      sprintf(stringBuff,"%02x ",EEPROM.read(c+d)); //get char
      Serial.print(stringBuff);
    }
    Serial.print("   ");
    // Now do the ASCII equivs of the values.
    for(int d=0;d<16;d++){
      ourChar = EEPROM.read(c+d);
      if (! isprint(ourChar)) {
        Serial.print(".");
      }
      else {
        sprintf(stringBuff,"%c",EEPROM.read(c+d));
        Serial.print(stringBuff);
      }
    }
    Serial.println(""); 
  }
  Serial.println(); // Make sure we set the cursor to a new line when we exit.
}



void dumpDictToSerialChannel()
{
  /*
  Dumps a numbered list of dictionary words to the serial channel.
   There must be AT LEAST ONE WORD in the dictionary for this to work
   properly.
   */
  char msg[5];
  int c=0;
  int wordCount=1;

  sayMessage(0,2,true, true);
  sprintf(msg,"%02d) ",wordCount);
  Serial.print(msg);
  for (c=returnDictionaryBase();c <=(EEPROM_SIZE -1);c++){
    byte b = EEPROM.read(c);
    // Are we at the end of the table? 
    if (b == 255)  {
      break; // If this byte is 255, then we are at the end of the dict, breakout.
    }
    Serial.print(byte(b & 0x7F)); // Print the letter anyway.
    if (b > 128) {
      // This byte is end of a word. Do a new line anyway.
      if (((EEPROM.read(c+1) == 255)) || (c == (EEPROM_SIZE -1))){
        // If the next byte is FF, or we are at the end of the EEPROM
        Serial.println("");
        break; // Bale out of this loop.
      }
      // More to do. Print the next number.
      wordCount++;
      sprintf(msg,"\n\r%02d) ",wordCount);
      Serial.print(msg); 
    }
  }
  sayFreeSpace();
}

int returnDictWordCount(){
  int wordCount =0;
  for (int a=returnDictionaryBase();a<EEPROM_SIZE;a++) {
    if (EEPROM.read(a) == 255) {
      return(wordCount);
    }
    if (bitRead(EEPROM.read(a),7) == 1) {
      wordCount++;
    }
  }
  return(wordCount);
}


int getDictWord(int wordNum, char* byteArray){
  /*
  This function returns the wordNumth word in the dictionary
   as a string returned in byteArray. Returns 0 if successful
   or -1 if the dictionary contains fewer than wordNum entries.
   */

  // Step through EEPROM until we meet the wordNum-1 th byte 
  // with its bit 7 set
  int w=1;
  byte retVal[20];
  byte tempByte=0;

  for (int x = returnDictionaryBase(); x< EEPROM_SIZE;x++){
    // Are we on the word we want to use, and is it valid?
    if ((EEPROM.read(x) != 255) && (w == wordNum)) {
      // Yes, we are. Copy the word into the return value. 
      int ptr=0;
      do {
        // get a byte.
        tempByte = EEPROM.read(x++);
        // If this one is the end byte fix it and place it then exit.
        if (bitRead(tempByte,7) != 0) {
          bitClear(tempByte,7);
          byteArray[ptr++]=tempByte;
          byteArray[ptr]=0;
          return(0);
        }
        else {
          // This is not the last byte of the word, so just place it.
          byteArray[ptr++]=tempByte;
        }
      } 
      while(bitRead(tempByte,7)==0);
    }
    // This was not the word we wanted. Step to the end of it.
    while((x<EEPROM_SIZE) && (bitRead(EEPROM.read(x),7) ==0)) {
      x++;
    }
    // Bump the word counter
    w++;
  }
  return(-1); // We never found the word. return an error. 
}


int doInputBufferParser() {
  /*
   This is a function to parse the contents of the globally declared input
   buffer. It invokes functions which implement the various supported three
   letter commands. 
   */
  stringBuff[0]=0;

  if   (strncmp(inputBuffer,"HEL",3) == 0 ) {
    // Just spit out the help message as one message catalog entry.
    sayMessage(1,0,true, true);
    return(0);
  }

  if (strncmp(inputBuffer,"ADD",3) == 0 ) {
    /*
    It's an add command. So, init the stringBuff, and send
     the rest of it off to addToDictionary
     */
    stringBuff[0]=0;
    int w = addToDictionary(strncat(stringBuff,inputBuffer+4,strlen(inputBuffer)-4));

    // Put out a message based on the outcome of addToDictionary
    // this all makes sense if you cross-refer to catBuilder.pde
    switch (w) {

    case 0: 
      sayMessage(0,5,true,true);
      sayFreeSpace(); 
      break;

    case -1:
      sayMessage(0,7,true, true); 
      break;

    case -2:
      sayMessage(0,6,true, true); 
      sayMessage(0,8,true, true); 
      break;

    case -3:
      sayMessage(0,9,true, false); 
      Serial.println(MIN_WORDLENGTH,DEC);
      break;

    case -4:
      sayMessage(0,13,true, true); 
      break;


    default:
      sayMessage(0,10,true, true); 
      Serial.println(w,DEC);
      break;
    }
    return(0);
  }

  if (strncmp(inputBuffer,"DEP",3) == 0 ) {
    // It's a Dump EEPROM command. 
    dumpEEPROMToSerialChannel();
    return(0);
  }

  if (strncmp(inputBuffer,"DIC",3) == 0 ) {
    // It's a Dump Dictionary command. 
    dumpDictToSerialChannel();
    return(0);
  }

  if (strncmp(inputBuffer,"SWC",3) == 0 ) {
    // It's a show word Count command. 
    Serial.print(returnDictWordCount());
    sayMessage(0,3,true, true);  

    return(0);
  }

  if (strncmp(inputBuffer,"LEV",3) == 0 ) {
    /*
    It's a LEVel command. They can supply an arg to change the mode, or just
     use the command alone to see the curret level number to the serial channel.
     */
    stringBuff[0]=0;

    if (strlen(inputBuffer) <4){
      // Just the command then.
      Serial.print("Level = ");
      Serial.println(currentLevel,DEC);
      return(0);
    }
    else
    { 
      int w = atoi(strncat(stringBuff,inputBuffer+4,strlen(inputBuffer)-4));
      if ((w >=0) && (w <=MAX_LEVEL)) {
        // It's got an arg and it's legal - set the new game mode.
        currentLevel = w;
        Serial.print("Game level is now = ");
        Serial.println(currentLevel);
        lcd.clear();
      }
      else
      {
        Serial.println("Illegal level number.");
      }
      return(0);
    }
  }

  if (strncmp(inputBuffer,"DEL",3) == 0 ) {
    // It's a delete word command
    boolean res = doDeleteWord(strncat(stringBuff,inputBuffer+4,strlen(inputBuffer)-4));
    return(0);
  }

  Serial.flush();
  sayMessage(0,11,true, false); 
  Serial.println(inputBuffer);
}


int processKeyboardChar(char theChar) {
  /*
    This function actions an input character. Buffers the char if it's
   a printable one. If it's a control char, then actions it or ignores
   it. If it's a CR char, then returns 1, else it returns zero.
   */
  if ((32==theChar) || (theChar=='*') || (isalnum(theChar))){
    /*
    This is a printable char just buffer and echo the char if that
     would not make the string too long. Convert to upper case if 
     alphabetic.
     */
    if (isalpha(theChar)){
      bitClear(theChar,5);
    }
    if (inputBufferPtr < sizeof(inputBuffer)){
      inputBuffer[inputBufferPtr++]=theChar;
      inputBuffer[inputBufferPtr]=0; 
      Serial.print(theChar);
      return(0);
    }
  }  
  else
  {
    // It's a control character
    switch  (theChar) {

    case 13: // Carriage return (new line)
      return(1);
      break;

    case 18: // CTRL/R
      Serial.println("^R"); // New line
      Serial.print(inputBuffer);
      break;

    case 8: // Backspace
      // Only process this if there is something in the buffer.
      if (inputBufferPtr > 0) {
        inputBuffer[--inputBufferPtr]=0; // Kill the char in the buffer.
        Serial.print(theChar); // Send the backspace on screen
        Serial.print(" ");
        Serial.print(theChar);
      }
      break;
    }
    return(0);
  }
}

/*
   These notes describe the format of the EEPROM which has been filled up by the companion
 program "catBuilder". The notes in the source of that program explain why it is needed.
 The EEPROM format is as follows:
 
 Bytes 0&1 = EEPROM address of the message catalogue (low byte first).
 Bytes 2&3 = EEPROM address of the help message catalogue
 Bytes 4&5 = EEPROM address of the start of the dictionary
 
 The message catalogue, the help text and the dictionary are all simply sets of 
 in-memory strings in which the last byte has got bit 7 set. A byte containing 255
 indicates the end of a catalogue/dictionary.
 
 The following functions access or manage these structures. For the purposes of common
 coding, the message catalogue is referred to as cat#0, the help as cat#1 and the
 dictionary as cat#2
 */

char* sayMessage(int catNumber, int msgNumber, boolean outMsg, boolean doNewline) {
  word ePtr = 0;
  byte aChar=0;
  char* retVal;

  // Set the start address of the message catalogue in EEPROM
  switch (catNumber) {

  case 0: // Its the general messages
    ePtr += EEPROM.read(0);
    ePtr +=(EEPROM.read(1) <<8);
    break;

  case 1:
    ePtr += EEPROM.read(2);
    ePtr +=(EEPROM.read(3) <<8);
    break;

  }

  for (int msgCtr=0;msgCtr<=MAX_MESSAGES;msgCtr++){ 
    if (msgNumber == msgCtr) {
      boolean isLast=false;
      do { // This is the message, spit it out.
        aChar=EEPROM.read(ePtr);
        if (EEPROM.read(ePtr++) > 128) {
          isLast = true;  
          aChar=aChar-128; // If this is the marker byte - correct it for printing
        }
        if (outMsg == true) {
          Serial.print((aChar));
        }

      } 
      while(isLast == false);
      if (doNewline) {
        Serial.println("");
      }
      return(retVal);
    }
    // This was not the item we want, step on until we find the next item.
    do {
      aChar = EEPROM.read(ePtr++);
    } 
    while (aChar <128);
    /*
      Now ePtr points to the next item , or does it point to 255
     indicating that the message was not found?
     */
    if (aChar==255){
      Serial.println("Message not found!");
      return(retVal);
    }
  }
}


char* returnZeroPackedNumberString(int number, int desiredNumberLength) {
  /* 
   In C we would use the printf functions to provide additional zeroes to align
   columns of multi digit numbers (such as 001 and 133). Arduino doesn't provide 
   such functions in Serial.print and so this function returns the required leading
   zeroes for any desired number length. For example if provide with number=4 and
   desiredNumberLength = 3 it returns 004. Or, if provided with 100 and length of
   2 it returns 100 (i.e. non-sensible combinations are ignored).
   */
  char result[10];
  sprintf(result,"%04d",number);
  Serial.println(result);
  return(result);
}



int returnFreeEEPROMSpace(int EEPROM_LEN) {
  int lp;
  for (lp=(EEPROM_LEN-1);lp > 0; lp--){
    if (EEPROM.read(lp) != 255) {
      break;
    }
  }
  /*
    When we get here we know the EEPROM address of the highest byte that
   does NOT contain FF. Return the EEPROM_LEN minus that value.
   */
  return(EEPROM_LEN - (lp+1));
}


void sayFreeSpace(){
  sayMessage(0,4,true, false); 
  Serial.println(returnFreeEEPROMSpace(EEPROM_SIZE) ,DEC);
}

boolean doDeleteWord(char* wordToZap){
  /* 
   This function deletes a word - or all words - from the EEPROM dictionary
   space. It returns a boolean value indicating the outcome. True means
   the delete happened.
   */
  int thisTemp, nextWord, lp = 0;

  if (strlen(wordToZap) == 0){
    sayMessage(0,12,true, true); // Say missing arg
    return(false);
  }
  if (*wordToZap == '*') {
    Serial.print("Are you sure? ");
    while(Serial.available()==0){
    }
    if (toupper(Serial.read()) == 'Y') {
      thisTemp=returnDictionaryBase();
      format_EEPROM(EEPROM_SIZE,thisTemp);
      Serial.println("Zapping all words");
      return(true);
    }

    else 
      return(false); // They didn't type Y
  }
  // Okay, so not a wildcard delete, just one word. Does the word exist?
  thisTemp = returnWordLocation(wordToZap);

  if (thisTemp <0) {
    sayMessage(0,12,true, true); // Say bad arg
    return(false);
  }

  /* 
   Now we know the address of the word to be deleted. 
   We simply find the end of the word, then we copy everything
   from that point forward to the end of the EEPROM. NOTE
   this assumes that the word dictionary is the LAST catalogue
   in the EEPROM. This will break if that assumption no longer
   holds true.
   */

  // Find the end of this word.
  for (nextWord=thisTemp; nextWord < (EEPROM_SIZE -1); nextWord++){
    if ((EEPROM.read(nextWord) & 0x80) !=0){
      // Okay this one is the end char. Bump counter to the next char and break
      nextWord+=1;
      break;
    }
  }
  /*
    Now nextWord points to the first char of the next word, and thisTemp 
   points to the word to be erased.
   */
  for (lp=0;(nextWord+lp) < (EEPROM_SIZE -1); lp++){
    EEPROM.write(thisTemp + lp,EEPROM.read(nextWord+lp));
  }  
}


int returnWordLocation(char* theWord) {
  // Returns the offset into the EEPROM where the provided word begins.
  // returns -1 if the word was not found.
  char tChar;
  int matchCount=0;
  int wordStart = returnDictionaryBase();

  *theWord = toupper(*theWord);
  for (int ptr=returnDictionaryBase(); ptr < (EEPROM_SIZE-1);ptr++) {
    tChar=EEPROM.read(ptr);
    tChar=tChar & 0x7f;

    if (theWord[matchCount++] != tChar){
      // This char does NOT match. Advance ptr to the next word, if there is one.
      do
      { 
        ++ptr; 
      } 
      while (EEPROM.read(ptr)< 128);

      if (EEPROM.read(ptr)==0xFF) {
        // We reached the end of the populated area, return a fail
        return(-1);
      }
      matchCount=0;
      wordStart=ptr+1;
    }
    else
    {
      // it matches
      if (matchCount == strlen(theWord)) {
        // This is it then! Return the wordStart
        return(wordStart);
      }   
    }
  }
  // If we complete the loop then we didn't find the word and return -1
  return(-1);
}

int returnDictionaryBase() {
  // This function retrieves the base offset of the word dictionary. This is obtained
  // by reading the vectors written into the EEPROM by the CatBuilder program.
  int baseAddr=0;
  baseAddr=EEPROM.read(4);
  baseAddr+=(EEPROM.read(5)<<8);
  return(baseAddr);
}

void doLevelIntroducer(int levelNum, int wordCount) {

  int colCount=0;

  lcd.clear();  
  lcd.print("Find "); 
  lcd.print(wordCount); 
  lcd.print(" words hidden");  
  lcd.setCursor(0,1); 
  lcd.print("in the WordDune!"); 
  lcd.setCursor(0,2);
  lcd.print("Click for level "); 
  lcd.print(levelNum);
  while (awaitKeypress(35,true,8) == false){
    lcd.setCursor((colCount++ % 19),3);
    lcd.print(">");
    if (colCount>20) {
      for (colCount = 19;colCount>0;colCount--){
        delay(20-colCount);
        lcd.setCursor(colCount,3);
        lcd.print(" ");
      }
    }
  }
  randomSeed(colCount);
  lcd.clear();
}


int selectFourWords(){
  /*
    A function to select four unique words at random from the 
   current dictionary and store their catalogue numbers in the
   duneWords array. There needs to be a minimum four words in
   the dictionary for this to work. It returns -1 if there are
   too few words.
   */
  int lp,thisInt=0;
  int ranNum=0;

  randomSeed(millis());

  // Because we have to use 0 as a valid value, we start them all as -1
  for (lp=0;lp<=(WORDS_PER_LEVEL - 1);lp++){
    duneWords[lp].wordIndex = -1;
  }

  if (returnDictWordCount() < 4) {
    return(-1);
  }

  for (lp=0;lp<=(WORDS_PER_LEVEL -1);lp++) {
    do  {
      ranNum=random(1,1+(returnDictWordCount()));
    } 
    while(findNumberInWordArray(ranNum) == true);
    duneWords[lp].wordIndex = ranNum;
    duneWords[lp].tryCount=0;
  }
  showDuneWords(true,false,false);
}

boolean findNumberInWordArray(int numToFind){
  // returns true if the word array already contains the supplied number.
  for (int myLoop=0;myLoop<=(WORDS_PER_LEVEL -1);myLoop++){
    if (duneWords[myLoop].wordIndex == numToFind){
      // Already got this one
      return (true);
    }
  }
  return(false);
}

int chooseRandomWordIndexIndex(int wordCount) {
  /*
    This function returns the index of a duneWord that has not yet been shown four times 
   in the current level. It bumps the counter of the word to reflect that it will be
   shown. wordCount is the highest numbered member of the duneWords struct array.
   returns -1 if all the duneWords have been shown 4 times.
   */
  int randNum = 0;
  int lp=0;
  if (returnTotalTryCount() >= (levelData[currentLevel].showWordCount * (wordCount+1))){
    return (-1);
  }

  // Okay, there should be at least one showing left to do.
  do {
    // choose a random word from 0 to wordCount
    randNum = random(0,wordCount+1);

    // Has this one been shown enough? loop again if so. Else drop through. 
  } 
  while(duneWords[randNum].tryCount >= levelData[currentLevel].showWordCount);

  duneWords[randNum].tryCount++;
  return(randNum);
}

int returnTotalTryCount() {
  /*
   Returns an integer indicating the total number of word shows that have been done
   on this level
   */
  int tot =0;

  for (int lp=0;lp<=(WORDS_PER_LEVEL-1);lp++){
    tot += duneWords[lp].tryCount;
  }
  return tot;
}

void doLevelEnd() {
  /*
    Does the things we need to do to end off a level. Shows the user
   the list of duneWords they were trying to spot and invites them to
   move on up.
   */
  lcd.clear();
  sprintf(stringBuff,"LEVEL %d Complete: ",currentLevel);
  sayThis(stringBuff,true,true,false) ;  
  lcd.setCursor(0,1);
  sayThis("Click for word list",true,true,true);
  awaitKeypress(0,false,8); // Await key press
  showDuneWords(true,true,true);
  awaitKeypress(100000,false,8); // Await key press  
}

void showDuneWords(boolean onSerial, boolean onLCD, boolean pastTense){
  /*
   Function to show the current set of duneWords buried in the dune. Used by debug,
   end of level and command via USB reoutines. the list is sent to Serial and
   or LCD as indicated by the booleans supplied as args. The Serial port version
   also gets dictionary matrix indices to assist in debug purposes. In order not to
   have to repeat strings within the code, uses the companion function sayThis.
   */

  int thisInt=0;
  int lcdLine = -1;
  if (onLCD == true) {
    lcd.clear();
  }
  if (onSerial == true) {
    Serial.println("");
  }
  if (pastTense != true) {
    sayThis("Dune Words Are",onSerial,onLCD,true);
  }
  else
  {
    sayThis("Dune Words Were",onSerial,onLCD,true);    
  }
  delay(1500);
  lcd.clear();

  for (int thisLoop=0;thisLoop<WORDS_PER_LEVEL;thisLoop++){
    lcd.setCursor(0,++lcdLine);
    thisInt = getDictWord(duneWords[thisLoop].wordIndex, stringBuff);
    sayThis(stringBuff,onSerial,onLCD,true);
  }
}


void sayThis(char* whatToSay, boolean onSerial, boolean onLCD, boolean serialGetsNewline){
  /*
    A function to put out a supplied message string on the LCD screen 
   and or the serial channel. Booleans supplied by the caller decide which
   screens are sent this message. Also, a boolean decides if the serial
   screen is sent a newline after the supplied msg text.
   
   Assumes that the LCD cursor has been set to the desired print loc
   */
  if (onLCD == true) {
    lcd.print(whatToSay);
  }
  if (onSerial == true) {
    Serial.print(whatToSay);
    if (serialGetsNewline == true) {
      Serial.println("");
    }
  }
}

boolean awaitKeypress(unsigned long timeoutMs, boolean returnWhenPressed, int digitalPinNum){
  /*
   This function awaits a keypress on the specified digital pin for the specified
   amount of time. It returns true when a press is detected (see below) or false
   if there was no press within the timeout period. If timeoutMs is sent == 0 then
   it waits forever.
   
   PressDetection: The function reads the specified digital pin and takes its initial
   state to be the "unpressed" state. This allows active low or active high to be used.
   If the boolean returnWhenPressed is true, then the function returns true to the caller
   when the state of the pin changes (after a debounce delay). If that returnWhenPressed
   boolean is sent as false, then the function waits until the button is released (again 
   with a 10ms debounce delay) before returning true to the caller.
   */
  unsigned long startTime = millis(); // Everybody remember when we started.
  int initialState = digitalRead(digitalPinNum); // Initial state of the switch port bit
  int versDone=false;
  while (initialState == digitalRead(digitalPinNum)){
    doSerialInput(); 
    // Wait for the pin state to change, but timeout if it doesn't and timeouts are enabled
    if ((timeoutMs !=0) && ((startTime + timeoutMs) < millis())) {
      return(false);
    }
  }

  // The pin state has changed. If that's all we need, then debounce and return true
  delay(DEBOUNCE_DELAY);
  if (returnWhenPressed == true) {
    return (true);
  }
  // Else wait for release of the key
  startTime=millis();
  while (initialState != digitalRead(digitalPinNum)){
    delay(DEBOUNCE_DELAY);
   }   
  // Okay, the pin changed state again. Debounce delay and exit as before.

  return(true);
}

void doSplashScreen(){
  /*
    Here is a function to draw a splash screen for WordDune. It fills the
   screen with random chars and then writes "WordDune" at a random 
   location. It then quickly erases all the chars in the screen in one of
   two randomly chosen and exciting splash screen patterns leaving only
   the WordDune locations. It then capitalises the WordDune string, chooses
   a spare screen line to put a "click to start" msg on and awaits a
   keypress before returning.
   */
  int c,r,ctr,tgt_c, tgt_r=0;
  int coin;
  boolean bitMap[80];
  boolean readyToGo = false;
  do {
    lcd.clear();
    fillScreenWithRandomChars();
    for (r=0;r<=79;r++) {
      bitMap[r]=false;
    }

    // Now put the game name on screen at a random (fittable) location.
    // Everybody remember where we parked using r and c. 
    c=random(0,(19-8));
    r=random(0,4);
    lcd.setCursor(c,r);
    lcd.print("WordDune");
    delay(1000);

    // Two effects. Which one is chosen at random each time.
    coin=random(0,2); //flip a coin.


    if (coin == 1) {
      // randomly erase all locations except where the logo is.
      ctr=((4 * 20) - 8); // This is how many chars we have to blank out
      do {
        if (ctr > 10){
          delay(3); // Gawping time, but not if we're nearly there.
        }
        // Choose the column & row
        tgt_c = random(0,20);
        tgt_r=random(0,4);
        // If this is the logo row, and we are within it's cols, then do nothing.
        if ((tgt_r == r) && (tgt_c >= c) && (tgt_c <=c+7)){
          // Discard this one. Its on a column that'd erase the game name
        }
        else
        {
          // Okay this one is spatially legal, has it been done before?
          if (bitMap[(tgt_r*20)+tgt_c] == false) {
            // No, this one can be done. Go for it.
            lcd.setCursor(tgt_c,tgt_r);
            lcd.print(" ");
            ctr--;
            bitMap[(tgt_r*20)+tgt_c]=true;
          }
        }

      }
      while(ctr > 0);
    } 
    else // If coin flip was 0
    {
      for (tgt_r=0;tgt_r <4;tgt_r++) {
        for (tgt_c=0;tgt_c < 20; tgt_c++) {
          // Need an awaitKeyPress here to allow clicking when gawping.
          if ((tgt_r == r) && ((tgt_c >= c) && (tgt_c <=c+7))) {
            // do nothing. We're in the logo zone.
          }
          else
          {
            lcd.setCursor(tgt_c,tgt_r);
            lcd.print(" ");
            delay(7);
          }
        }
      }
      delay(20);
    }
    lcd.setCursor(c,r);
    lcd.print("WORDDUNE");
    tgt_r=(r+1) %4; // Put the cursor on a line which is NOT in use.
    lcd.setCursor(3,tgt_r);
    lcd.print("Click to START");

    if (awaitKeypress(random(5000,15000),true,8) == true){
      readyToGo = true;
    }
    randomSeed(millis()); 
  } 

  while (readyToGo == false);
}

void fillScreenWithRandomChars(){
  lcd.clear();
  for (int r=0;r<4;r++) {
    for (int c=0;c<20;c++) {
      lcd.setCursor(c,r);
      lcd.print( byte(random(65,(65+26))));      
    }
  }
}


void doSerialInput(){
  byte aChar;
  // Check for a Serial command.
  if (Serial.available() >0) {
    // get the char
    aChar=Serial.read();
    if (processKeyboardChar(aChar)){
      // It was a terminator. Tail the buffer and parse it
      Serial.println();
      inputBuffer[inputBufferPtr++]=0;
      doInputBufferParser();
      inputBufferPtr=0;
      inputBuffer[0]=0;
      Serial.print(prompt);
    }
  }
}


