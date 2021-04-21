#include <Arduino.h>
#line 1 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
#include <SoftwareSerial.h>
#include "RandomizePPC.h"
#include "sha1.h"

namespace /* anonymous */ {

  #define RX 3     // physical pin 2 of attiny85
  #define TX 4     // physical pin 3 of attiny85
  #define LED 13  // physical pin 5 of attiny85

  SoftwareSerial mySerial(RX, TX);
  Sha1Class SHA1;

  const uint8_t MY_TINY_ID = SER_NO;

  // NOTE: Just trying to get it working right now,
  //  will work on removing String() variables later...
  const char* CMD_RECV = ">>>";
  const char* CMD_SEND = "<<<";
  const char  CMD_SEP = ',';
  const char  CMD_END = '\n';

  unsigned int cmd_id, diff, iJob = 0;
  String cmd_code, hash, job, result;
  bool new_job_received = false;

  unsigned int STATE = LOW;
  unsigned int FAILED_LOOPS = 0;

  int timer = 0;
  void setupTimerHook(void) {
    TCCR0A=(1<<WGM01);    //Set the CTC mode   
    OCR0A=0xF9; //Value for ORC0A for 1ms 
    
    TIMSK0|=(1<<OCIE0A);   //Set the interrupt request
    sei(); //Enable interrupt
    
    TCCR0B|=(1<<CS01);    //Set the prescale 1/64 clock
    TCCR0B|=(1<<CS00);
  }

  ISR(TIMER0_COMPA_vect){    //This is the interrupt request
    timer++;
  }

  void setupPinModes(void) {
    pinMode(RX, INPUT);
    pinMode(TX, OUTPUT);
    
    mySerial.begin(9600);
  }

  bool handleSerialInput(void) {
    bool result = false;

    while (mySerial.available() > 0) {
      // It's not the start command, move on to next command...
      cmd_code = mySerial.readStringUntil(CMD_SEP);
      if (cmd_code != CMD_RECV) {
        mySerial.readStringUntil(CMD_END);
        continue;
      }

      // It's not meant for me, move on to the next command...
      cmd_id = mySerial.readStringUntil(CMD_SEP).toInt();
      if (cmd_id != MY_TINY_ID) {
        mySerial.readStringUntil(CMD_END);
        continue;
      }

      hash = mySerial.readStringUntil(CMD_SEP);
      job = mySerial.readStringUntil(CMD_SEP);
      diff = (mySerial.readStringUntil(CMD_END).toInt() * 100) + 1;
      new_job_received = true;
      digitalWrite(LED, STATE ^= HIGH);
      result = true;
      break;
    }

   return result;
  }

  void sendResultsBack() {
    mySerial.print(CMD_SEND+CMD_SEP+MY_TINY_ID+CMD_SEP+iJob+CMD_END);
    digitalWrite(LED, STATE ^= HIGH);
  }

  void clearResultsValues() {
    iJob = diff = 0;
    hash = job = result = "";
    new_job_received = false;
  }

  String printHash(uint8_t* hash) {
    String result = "";

    for (int i=0; i<20; i++) {
      result += ("0123456789abcdef"[hash[i]>>4]);
      result += ("0123456789abcdef"[hash[i]&0xf]);
    }
    return result;
  }
}

#line 105 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
void setup();
#line 111 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
void loop();
#line 105 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
void setup() {
  setupTimerHook();
  setupPinModes();
  sendResultsBack(); 
}

void loop() {
  if (!handleSerialInput()) {
    // Avoiding the use of millis() if at all possible - saves code-space
    if (timer > 5000) {
      timer = 0;
      sendResultsBack();
    }
    return;
  }

  timer = 0;
  for (iJob = 0; iJob < diff; iJob++) { // Difficulty loop
    SHA1.init();
    SHA1.print(hash + String(iJob));
    result = printHash(SHA1.result());

    if (result == job) {
      sendResultsBack();
      clearResultsValues();
      break; // Stop and ask for more work
    }
  }
}

