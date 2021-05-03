# 1 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
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
# 16 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
# 17 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 2


# 18 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
//*************************************************
// NOTE: You MUST change the value of the #define
//  _SS_MAX_RX_BUFF from 64 to any other value
//  that is less than 64. For development of 
//  this ATTiny85 miner, I have been able to use
//  a value of 8 successfully and settled on a
//  value of 4. IE: #define __SS_MAX_RX_BUFF  4
//  Within the <SoftwareSerial.h> header file.
# 27 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 2

//*************************************************
// Uncomment this line when you want to use any
// of the debugPrint() / debugPrintln() functions
// for troubleshooting code changes
// #define _DEBUG
# 34 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 2

# 36 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 2

namespace /* anonymous */ {





  SoftwareSerial mySerial(3 /* physical pin 2 of attiny85*/, 4 /* physical pin 3 of attiny85*/);

  // Serial tokens






  // Serial input data



  /*

               1         2         3         4         5         6         7         8         9

     01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234

     2,45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,4200000000~

  */
# 62 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
  // define byte positions of various components of inputBuffer...
# 72 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
  char inputBuffer[95];
  char *tinyIdValue = &inputBuffer[0];
  char *hashValue = &inputBuffer[2];
  char *jobValue = &inputBuffer[43];
  char *diffValue = &inputBuffer[84];

  char *abbrHashValue = &inputBuffer[2];
  char *abbrJobValue = &inputBuffer[22];
  char *workBuffer = &inputBuffer[43];
  char *diffBuffer = &inputBuffer[84 - 1];
  uint8_t chip_id;
  unsigned difficulty, iJob;

  boolean readInProgress = false;
  boolean newDataFromPC = false;
  boolean isAlreadyWorking = false;

  uint8_t bytesReceived;

  Sha1Class SHA1;

  void setupPinModes(void) {
    pinMode(3 /* physical pin 2 of attiny85*/, 0x0);
    pinMode(4 /* physical pin 3 of attiny85*/, 0x1);
    pinMode(
# 96 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
           0 
# 96 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
           /* physical pin 5 of attiny85*/, 0x1);

    mySerial.begin(9600);
  }

  void turnLED_On(void) {
    digitalWrite(
# 102 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
                0 
# 102 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
                /* physical pin 5 of attiny85*/, 0x1);
  }

  void turnLED_Off(void) {
    digitalWrite(
# 106 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
                0 
# 106 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
                /* physical pin 5 of attiny85*/, 0x0);
  }

  void setNullCharacters(void) {
    inputBuffer[1] = '\0';
    inputBuffer[42] = '\0';
    inputBuffer[83] = '\0';
    inputBuffer[94] = '\0';
  }

  void printHash(uint8_t *hash, uint8_t *target) {
    int out_pos = 0;

    // note: lower case letters only...
    for (uint8_t i=0; i<20; ++i) {
      target[out_pos++] = ("0123456789abcdef"[hash[i]>>4]);
      target[out_pos++] = ("0123456789abcdef"[hash[i]&0xf]);
    }
  }

  void compressHashString(uint8_t *hash, uint8_t *target) {
    uint8_t in_pos, out_pos;
    uint8_t hi, lo;

    // note: lower case letters only...
    for (in_pos = 0; in_pos < (20 << 1); in_pos += 2 ) {
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

    if (4 != chip_id) {
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

    ;
    // debugPrintln(workBuffer);
  }

  void readInputSerialData() {
    if(mySerial.available() > 0) {
      char x = mySerial.read();

      if (x == '\n') {
        readInProgress = false;
        newDataFromPC = true;
        parseData();
      }

      if(readInProgress) {
        inputBuffer[bytesReceived] = x;
        bytesReceived++;
        if (bytesReceived == 95) {
          bytesReceived = 95 - 1;
        }
      }

      if (x == '>') {
        inputBuffer[0] = '\0';
        bytesReceived = 0;
        readInProgress = true;
      }
    }
  }

  void sendResultsData() {
    mySerial.print('<');
    mySerial.print(4);
    mySerial.print(',');
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

        if (memcmp(SHA1.result(), abbrJobValue, 20) == 0) {
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

void setup() {
  setupPinModes();
}

void loop() {
  readInputSerialData();
  findHashSolution();
}
