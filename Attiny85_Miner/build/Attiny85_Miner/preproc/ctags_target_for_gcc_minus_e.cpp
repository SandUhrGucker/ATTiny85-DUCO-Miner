# 1 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
# 2 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 2
# 3 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 2
# 4 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 2

namespace /* anonymous */ {





  SoftwareSerial mySerial(3 /* physical pin 2 of attiny85*/, 4 /* physical pin 3 of attiny85*/);
  Sha1Class SHA1;

  const uint8_t MY_TINY_ID = (((((z=36969*(z&65535)+(z>>16))<<16)+((w=18000*(w&65535)+(w>>16))&65535))^(jcong=69069*jcong+1234567))+(jsr=(jsr=(jsr=jsr^(jsr<<17))^(jsr>>13))^(jsr<<5)));

  // NOTE: Just trying to get it working right now,
  //  will work on removing String() variables later...
  const char* CMD_RECV = ">>>";
  const char* CMD_SEND = "<<<";
  const char CMD_SEP = ',';
  const char CMD_END = '\n';

  unsigned int cmd_id, diff, iJob = 0;
  String cmd_code, hash, job, result;
  bool new_job_received = false;

  unsigned int STATE = 0x0;
  unsigned int FAILED_LOOPS = 0;

  int timer = 0;
  void setupTimerHook(void) {
    
# 32 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
   (*(volatile uint8_t *)((0x24) + 0x20))
# 32 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
         =(1<<
# 32 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
              1
# 32 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
                   ); //Set the CTC mode   
    
# 33 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
   (*(volatile uint8_t *)((0x27) + 0x20))
# 33 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
        =0xF9; //Value for ORC0A for 1ms 

    
# 35 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
   (*(volatile uint8_t *)(0x6E))
# 35 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
         |=(1<<
# 35 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
               1
# 35 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
                     ); //Set the interrupt request
    
# 36 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
   __asm__ __volatile__ ("sei" ::: "memory")
# 36 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
        ; //Enable interrupt

    
# 38 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
   (*(volatile uint8_t *)((0x25) + 0x20))
# 38 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
         |=(1<<
# 38 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
               1
# 38 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
                   ); //Set the prescale 1/64 clock
    
# 39 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
   (*(volatile uint8_t *)((0x25) + 0x20))
# 39 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
         |=(1<<
# 39 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
               0
# 39 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
                   );
  }

  
# 42 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino" 3
 extern "C" void __vector_14 /* TimerCounter0 Compare Match A */ (void) __attribute__ ((signal,used, externally_visible)) ; void __vector_14 /* TimerCounter0 Compare Match A */ (void)
# 42 "d:\\projects\\DuinoMiners\\Attiny85\\Attiny85_Miner\\Attiny85_Miner.ino"
                       { //This is the interrupt request
    timer++;
  }

  void setupPinModes(void) {
    pinMode(3 /* physical pin 2 of attiny85*/, 0x0);
    pinMode(4 /* physical pin 3 of attiny85*/, 0x1);

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
      digitalWrite(13 /* physical pin 5 of attiny85*/, STATE ^= 0x1);
      result = true;
      break;
    }

   return result;
  }

  void sendResultsBack() {
    mySerial.print(CMD_SEND+CMD_SEP+MY_TINY_ID+CMD_SEP+iJob+CMD_END);
    digitalWrite(13 /* physical pin 5 of attiny85*/, STATE ^= 0x1);
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
