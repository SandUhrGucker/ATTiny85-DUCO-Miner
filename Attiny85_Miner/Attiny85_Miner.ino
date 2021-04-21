/*
Test data sent via Serial-Monitor:

>4,0000000000000000000000000000000000000000,13f23aa0bff731803eb9d4a000ce37d067301dd4,6
>5,13f23aa0bff731803eb9d4a000ce37d067301dd4,45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,6
>2,45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,6
>5,9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,9001c6ac34eb185cdb1cabde89acf4275f39b593,6

// Expected results:
<4,1
<5,8
<2,600
<5,480

*/

#include <SoftwareSerial.h>
#include "sha1.h"

namespace /* anonymous */ {

  #ifdef ARDUINO_AVR_UNO
    #define TESTING_ON_UNO
  #endif
  
  #ifdef TESTING_ON_UNO
    #define LED LED_BUILTIN 
    #define mySerial Serial
  #else
    #define RX 3     // physical pin 2 of attiny85
    #define TX 4     // physical pin 3 of attiny85
    #define LED PB0  // physical pin 5 of attiny85
    SoftwareSerial mySerial(RX, TX);
  #endif
  
  #define DELAYED_RESPONSE 5000
  
  const uint32_t MY_TINY_ID = 1; 

  // Serial tokens
  const char RECV_TOKEN = '>';
  const char SEND_TOKEN = '<';
  const char END_TOKEN = '\n';
  const char SEP_TOKEN = ',';
  
  // Serial input data
  String inputBuffer = "";
  boolean readInProgress = false;
  boolean newDataFromPC = false;

  // Serial parsed data
  String hash, job, result;
  uint32_t chip_id, iJob, diff;

  Sha1Class SHA1;
  
  int timer = 0;
  void setupTimerHook(void) {
    TCCR0A=(1<<WGM01);    //Set the CTC mode   
    OCR0A=0xF9; //Value for ORC0A for 1ms 

#ifdef TESTING_ON_UNO
    TIMSK0|=(1<<OCIE0A);   //Set the interrupt request
#else
    TIMSK|=(1<<OCIE0A);    //Set the interrupt request
#endif

    sei(); //Enable interrupt
    
    TCCR0B|=(1<<CS01);    //Set the prescale 1/64 clock
    TCCR0B|=(1<<CS00);
  }

  ISR(TIMER0_COMPA_vect){    //This is the interrupt request
    timer++;
  }

  void resetTimer(void) {
    cli();
    timer = 0;
    sei();
  }

  void turnLED_On(void) {
    digitalWrite(LED, HIGH);
  }

  void turnLED_Off(void) {
    digitalWrite(LED, LOW);
  }
  
  void setupPinModes(void) {
#ifndef TESTING_ON_UNO
    pinMode(RX, INPUT);
    pinMode(TX, OUTPUT);
#endif

    mySerial.begin(9600);
    turnLED_Off();
  }

  // https://stackoverflow.com/questions/9072320/split-string-into-string-array
  String getValue(String data, char separator, int index)
  {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;
  
    for(int i=0; i<=maxIndex && found<=index; i++){
      if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
    }
  
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
  }
  
  void parseData(void) {
    chip_id = getValue(inputBuffer, SEP_TOKEN, 0).toInt();
    if (chip_id != MY_TINY_ID) {
      newDataFromPC = false;
      return;
    }
      
    hash = getValue(inputBuffer, SEP_TOKEN, 1);
    job = getValue(inputBuffer, SEP_TOKEN, 2);
    diff = getValue(inputBuffer, SEP_TOKEN, 3).toInt();
    resetTimer();
    turnLED_On();
  }
    
  void getInputSerialData(void) {
    if (mySerial.available() > 0) {
      char x = mySerial.read();
  
      if (x == END_TOKEN) {
        readInProgress = false;
        newDataFromPC = true;
        inputBuffer += END_TOKEN;
        parseData();
      }
      
      if(readInProgress) {
        inputBuffer += x;
      }
  
      if (x == RECV_TOKEN) { 
        readInProgress = true;
      }
    }
  }

  void sendResultsData() {
    mySerial.print(SEND_TOKEN);
    mySerial.print(MY_TINY_ID);
    mySerial.print(SEP_TOKEN);
    mySerial.println(iJob);

    hash = job = result = "";
    inputBuffer = "";
    turnLED_Off();
  }

  void printHash(uint8_t* hash, String &result) {
    result = "";
    for (int i=0; i<20; i++) {
      result += ("0123456789abcdef"[hash[i]>>4]);
      result += ("0123456789abcdef"[hash[i]&0xf]);
    }
  }

  findHashSolution(void) {
    if (newDataFromPC) {
      for (iJob = 0; iJob < diff * 100 + 1; iJob++) { // Difficulty loop
        SHA1.init();
        SHA1.print(hash + String(iJob));
        printHash(SHA1.result(), result);
        
        if (result == job) {
          sendResultsData();
          resetTimer();
          break; // Stop and ask for more work
        }
      }
  
      newDataFromPC = false;
    }
  }

} // namespace...

void setup() {
  setupTimerHook();
  setupPinModes();
  
  // delay()...
  while (timer < (1000 * MY_TINY_ID));
  
  sendResultsData();
  resetTimer();
}

void loop() {
  getInputSerialData();
  findHashSolution();

  // If I've waited five seconds w/o a new job, request it again...
  if (timer >= DELAYED_RESPONSE) {
    sendResultsData();
    resetTimer();
  }
}
