#ifndef _DUCO_MINER_H__
#define _DUCO_MINER_H__

#include <arduino.h>
#include <ESP8266WiFi.h> // Include WiFi library

class DUCO_Miner {
    static unsigned int BASE_MINER_ID;

  public:
    void initialize(String host, const int port, String duco) {
      hostIP = host;
      hostPort = port;
      ducoUser = duco;      
      startTimer = micros();
      shares = rejects = 0;
      minerID = BASE_MINER_ID++;
    }

    void sendSolution(unsigned int value) {
      if (value == which) shares++;
        else rejects++;

      // requestNewJob();
    }

    void requestNewJob(void) {
      which = rand() % 4;
      buffer = jobs[which];

      Serial.print(">");
      Serial.print(minerID);
      Serial.print(",");
      Serial.println(buffer);
      startTimer = micros();      
    }

    unsigned long elapsedTime(void) const {
      return micros() - startTimer;
    }

  protected:
    WiFiClient wifiClient;

    String hostIP;
    int hostPort;
    String ducoUser;
    String buffer;
    
    unsigned long startTimer;
    unsigned int minerID;

    unsigned int shares;
    unsigned int rejects;

  private:
    int which;

    static String jobs[];
    static unsigned int answers[];

};

#endif // _DUCO_MINER_H__
