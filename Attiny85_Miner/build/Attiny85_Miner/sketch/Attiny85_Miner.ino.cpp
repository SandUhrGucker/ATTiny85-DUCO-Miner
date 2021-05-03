#include <Arduino.h>
#line 1 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
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
#include <stdlib.h>

//*************************************************
// NOTE: You MUST change the value of the #define
//  _SS_MAX_RX_BUFF from 64 to any other value
//  that is less than 64. For development of 
//  this ATTiny85 miner, I have been able to use
//  a value of 8 successfully and settled on a
//  value of 4. IE: #define __SS_MAX_RX_BUFF  4
//  Within the <SoftwareSerial.h> header file.
#include <SoftwareSerial.h> 

//*************************************************
// Uncomment this line when you want to use any
// of the debugPrint() / debugPrintln() functions
// for troubleshooting code changes
// #define _DEBUG
#include "debugging.h"

#include "sha1.h"

namespace /* anonymous */ {
  #define MY_TINY_ID   4

  #define RX 3     // physical pin 2 of attiny85
  #define TX 4     // physical pin 3 of attiny85
  #define LED PB0  // physical pin 5 of attiny85
  SoftwareSerial mySerial(RX, TX);

  // Serial tokens
  #define RECV_TOKEN  '>'
  #define SEND_TOKEN  '<'
  #define SEP_TOKEN   ','
  #define END_TOKEN   '\n'
  #define NULL_TOKEN  '\0'

  // Serial input data
  #define INPUT_BUFFER_SIZE   95
  #define HASH_BUFFER_SIZE    20

  /*
               1         2         3         4         5         6         7         8         9
     01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234
     2,45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,4200000000~
  */

  // define byte positions of various components of inputBuffer...
  #define TINY_ID_POSITION           0
  #define HASH_POSITION              2
  #define JOB_POSITION              43
  #define DIFF_POSITION             84
  #define ABBR_HASH_POSITION    HASH_POSITION
  #define ABBR_JOB_POSITION         22
  #define WORK_BUFFER_POSITION  JOB_POSITION
  #define DIFF_BUFFER           DIFF_POSITION - 1

  char inputBuffer[INPUT_BUFFER_SIZE];  
  char *tinyIdValue = &inputBuffer[TINY_ID_POSITION];
  char *hashValue = &inputBuffer[HASH_POSITION];
  char *jobValue = &inputBuffer[JOB_POSITION];
  char *diffValue = &inputBuffer[DIFF_POSITION];

  char *abbrHashValue = &inputBuffer[ABBR_HASH_POSITION];
  char *abbrJobValue = &inputBuffer[ABBR_JOB_POSITION];
  char *workBuffer = &inputBuffer[WORK_BUFFER_POSITION];
  char *diffBuffer = &inputBuffer[DIFF_BUFFER];
  uint8_t  chip_id;
  unsigned difficulty, iJob;

  boolean readInProgress = false;
  boolean newDataFromPC = false;
  boolean isAlreadyWorking = false;

  uint8_t bytesReceived;

  Sha1Class SHA1;

  void setupPinModes(void) {
    pinMode(RX, INPUT);
    pinMode(TX, OUTPUT);
    pinMode(LED, OUTPUT);

    mySerial.begin(9600);
  }

  void turnLED_On(void) {
    digitalWrite(LED, HIGH);
  }

  void turnLED_Off(void) {
    digitalWrite(LED, LOW);
  }

  void setNullCharacters(void) {
    inputBuffer[1]  = NULL_TOKEN;
    inputBuffer[42] = NULL_TOKEN;
    inputBuffer[83] = NULL_TOKEN;
    inputBuffer[94] = NULL_TOKEN;
  }

  void printHash(uint8_t *hash, uint8_t *target) {
    int out_pos = 0;

    // note: lower case letters only...
    for (uint8_t i=0; i<HASH_BUFFER_SIZE; ++i) {
      target[out_pos++] = ("0123456789abcdef"[hash[i]>>4]);
      target[out_pos++] = ("0123456789abcdef"[hash[i]&0xf]);
    }
  }

  void compressHashString(uint8_t *hash, uint8_t *target) {
    uint8_t in_pos, out_pos;
    uint8_t hi, lo;

    // note: lower case letters only...
    for (in_pos = 0; in_pos < (HASH_BUFFER_SIZE << 1); in_pos += 2 ) {
      hi = hash[in_pos] > '9' ? hash[in_pos] - 'a' + 10 : hash[in_pos] - '0';
      lo = hash[in_pos+1] > '9' ? hash[in_pos+1] - 'a' + 10 : hash[in_pos+1] - '0';
      target[out_pos++] = (hi << 4) | lo;
    }
  }
  
  void parseData(void) { 

    setNullCharacters();

    chip_id = atoi(inputBuffer);
    // debugPrint("  Chip-ID: "); debugPrintln(chip_id);

    difficulty = atoi(diffValue) * 100 + 1;
    // debugPrint("  Difficulty: "); debugPrintln(difficulty);

    if (MY_TINY_ID != chip_id) {
      // debugPrint("chip_id != MY_TINY_ID: ");
      // debugPrint(chip_id); debugPrint(" != "); debugPrintln(MY_TINY_ID);
      newDataFromPC = false;
      return;
    }

    // Step 1: Encode hashValue to make room for encoded jobValue...
    // debugPrint("Hash Value: "); debugPrintln(hashValue);
    compressHashString(hashValue, abbrHashValue);

    // Step 2: Encode jobValue and move it to upper half of hashValue...
    // debugPrint("Job value: "); debugPrintln(jobValue);
    compressHashString(jobValue, abbrJobValue);

    // Step 3: Build up the workBuffer...
    printHash(abbrHashValue, workBuffer);

    debugPrint("Work-Buffer: ");
    // debugPrintln(workBuffer);
  }

  void readInputSerialData() {
    if(mySerial.available() > 0) {
      char x = mySerial.read();
        
      if (x == END_TOKEN) {
        readInProgress = false;
        newDataFromPC = true;
        parseData();
      }
      
      if(readInProgress) {
        inputBuffer[bytesReceived] = x;
        bytesReceived++;
        if (bytesReceived == INPUT_BUFFER_SIZE) {
          bytesReceived = INPUT_BUFFER_SIZE - 1;
        }
      }
  
      if (x == RECV_TOKEN) { 
        inputBuffer[0] = '\0';
        bytesReceived = 0; 
        readInProgress = true;
      }
    }
  }

  void sendResultsData() {
    mySerial.print(SEND_TOKEN);
    mySerial.print(MY_TINY_ID);
    mySerial.print(SEP_TOKEN);
    mySerial.println(iJob);
  }

  void findHashSolution(void) {
    if (newDataFromPC) {
      isAlreadyWorking = true;

      turnLED_On(); 
      for (iJob = 0; iJob < difficulty; ++iJob) {
        // debugPrint("iJob: "); debugPrintln(iJob);
  
        SHA1.init();
        SHA1.print(workBuffer);
        SHA1.print(iJob);

        if (memcmp(SHA1.result(), abbrJobValue, HASH_BUFFER_SIZE) == 0) {
          // debugPrint("Result Found: "); debugPrintln(iJob);
          sendResultsData();
          break; // Stop and ask for more work
        }
      }

      turnLED_Off();

      newDataFromPC = false;
      isAlreadyWorking = false;
    }
  }

} // namespace...

#line 231 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
void setup();
#line 235 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
void loop();
#line 231 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
void setup() {
  setupPinModes();
}

void loop() {
  readInputSerialData();
  findHashSolution();
}

