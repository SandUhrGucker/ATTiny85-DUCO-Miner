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
#include <SoftwareSerial.h>

// #define _DEBUG
#include "debugging.h"

#include "sha1.h"

namespace /* anonymous */ {

  #define RX 3     // physical pin 2 of attiny85
  #define TX 4     // physical pin 3 of attiny85
  #define LED PB0  // physical pin 5 of attiny85
  SoftwareSerial mySerial(RX, TX);

  #define DELAYED_RESPONSE 5000
  
  #define MY_TINY_ID   1 

  // Serial tokens
  #define RECV_TOKEN  '>'
  #define SEND_TOKEN  '<'
  #define SEP_TOKEN   ','
  #define END_TOKEN   '\n'
  #define NULL_TOKEN  '\0'

  // Serial input data
  #define INPUT_BUFFER_SIZE   95
  #define HASH_BUFFER_SIZE    20
  #define DIFF_BUFFER_SIZE    10

  /*
               1         2         3         4         5         6         7         8         9
     01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234
     2,45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,4200000000~
  */
  #define TINY_ID_POSITION           0
  #define HASH_POSITION              2
  #define JOB_POSITION              43
  #define DIFF_POSITION             84
  #define ABBR_HASH_POSITION    HASH_POSITION
  #define ABBR_JOB_POSITION         22
  #define WORK_BUFFER_POSITION  JOB_POSITION
  #define DIFF_BUFFER           DIFF_POSITION - 1

  char inputBuffer[INPUT_BUFFER_SIZE];  
  uint8_t *tinyIdValue = &inputBuffer[TINY_ID_POSITION];
  uint8_t *hashValue = &inputBuffer[HASH_POSITION];
  uint8_t *jobValue = &inputBuffer[JOB_POSITION];
  uint8_t *diffValue = &inputBuffer[DIFF_POSITION];
  uint8_t *abbrHashValue = &inputBuffer[ABBR_HASH_POSITION];
  uint8_t *abbrJobValue = &inputBuffer[ABBR_JOB_POSITION];
  uint8_t *workBuffer = &inputBuffer[WORK_BUFFER_POSITION];
  uint8_t *diffBuffer = &inputBuffer[DIFF_BUFFER];
  uint8_t  chip_id;
  uint32_t difficulty;

  boolean readInProgress = false;
  boolean newDataFromPC = false;
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

    debugPrintln(inputBuffer);
    setNullCharacters();

    debugPrint("Chip-Id Value: "); debugPrint((char*) tinyIdValue);
    chip_id = atoi(tinyIdValue);
    debugPrint("  Chip-ID: "); debugPrintln(chip_id);

    debugPrint("Diff-Value: "); debugPrint((char*) diffValue);
    difficulty = atoi(diffValue) * 100 + 1;
    debugPrint("  Difficulty: "); debugPrintln(difficulty);

    if (MY_TINY_ID != chip_id) {
      debugPrint("chip_id != MY_TINY_ID: ");
      debugPrint(chip_id); debugPrint(" != "); debugPrintln(MY_TINY_ID);
      newDataFromPC = false;
      return;
    }

    // Step 1: Encode hashValue to make room for encoded jobValue...
    debugPrint("Hash Value: "); debugPrintln((char*) hashValue);
    compressHashString(hashValue, abbrHashValue);

    // Step 2: Encode jobValue and move it to upper half of hashValue...
    debugPrint("Job value: "); debugPrintln((char*) jobValue);
    compressHashString(jobValue, abbrJobValue);

    // Step 3: Build up the workBuffer...
    printHash(abbrHashValue, workBuffer);

    // Step 4: Append the difficulty value to the workBuffer...
    itoa(difficulty, diffBuffer, 10);

    debugPrint("Work-Buffer: ");
    debugPrintln((char*)workBuffer);

    turnLED_On();  
  }

  void readInputSerialData() {
    if(mySerial.available() > 0) {
      char x = mySerial.read();
        
      if (x == '\n') {
        readInProgress = false;
        newDataFromPC = true;
        // inputBuffer[bytesReceived] = '\n';
        parseData();
      }
      
      if(readInProgress) {
        inputBuffer[bytesReceived] = x;
        bytesReceived++;
        if (bytesReceived == INPUT_BUFFER_SIZE) {
          bytesReceived = INPUT_BUFFER_SIZE - 1;
        }
      }
  
      if (x == '>') { 
        inputBuffer[0] = '\0';
        bytesReceived = 0; 
        readInProgress = true;
      }
    }
  }

} // namespace...

void setup() {
  setupPinModes();

  debugPrintln("ATTiny85 Serial Miner setup complete!");
}

void loop() {
  readInputSerialData();
}
