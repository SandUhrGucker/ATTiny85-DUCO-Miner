//////////////////////////////////////////////////////////
//  _____        _                    _____      _
// |  __ \      (_)                  / ____|    (_)
// | |  | |_   _ _ _ __   ___ ______| |     ___  _ _ __
// | |  | | | | | | '_ \ / _ \______| |    / _ \| | '_ \
// | |__| | |_| | | | | | (_) |     | |___| (_) | | | | |
// |_____/ \__,_|_|_| |_|\___/       \_____\___/|_|_| |_|
//  Code for ESP8266 boards v2.3
//  © Duino-Coin Community 2019-2021
//  Distributed under MIT License
//////////////////////////////////////////////////////////
//  https://github.com/revoxhere/duino-coin - GitHub
//  https://duinocoin.com - Official Website
//  https://discord.gg/k48Ht5y - Discord
//  https://github.com/revoxhere - @revox
//  https://github.com/JoyBed - @JoyBed
//////////////////////////////////////////////////////////
//  If you don't know what to do, visit official website
//  and navigate to Getting Started page. Happy mining!
//////////////////////////////////////////////////////////
#include <ESP8266WiFi.h> // Include WiFi library
#include <ESP8266mDNS.h> // OTA libraries
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <ESPAsyncTCP.h>

#include <Crypto.h>  // experimental SHA1 crypto library
using namespace experimental::crypto;

#include "version-info.h"   // Created by calling increase-version.bat; requires
                            // increase-version.dat file which stores the current
                            // build number - Needed for VersionInfo() macro.
#include "wifi-manager.hpp" // contains #defines NETWORK_SSID / NETWORK_PWD and
                            // DUCO_USERNAME so my personal information isn't
                            // pushed to git repo since this file is .gitignore'd

#define WIFI_TIMEOUT 30000 // 5-minutes...
#define MAX_MINERS   5

namespace /* anonymous */ {
  const char* ssid          = NETWORK_SSID;    // Change this to your WiFi SSID
  const char* password      = NETWORK_PWD;     // Change this to your WiFi password
  const char* ducouser      = DOCU_USERNAME;   // Change this to your Duino-Coin username
  const char* rigIdentifier = "ATTiny85";        // Change this if you want a custom miner name

  const char * host = "51.15.127.80"; // Static server IP
  const int port = 2811;
  unsigned int Shares = 0; // Share variable

  const char RECV_TOKEN = '>';
  const char SEND_TOKEN = '<';
  const char END_TOKEN = '\n';
  const char SEP_TOKEN = ',';

  // Serial input data
  String inputBuffer = "";
  boolean serialReadInProgress = false;
  boolean newDataFromSerial = false;

  uint32_t miner_id;

  struct DUCO_Miner {
    AsyncClient *Client;
    String inputBUffer;
    uint32_t Solution;
  } Miners[MAX_MINERS];

  void SetupWifi() {
    Serial.println("Connecting to: " + String(ssid));
    WiFi.mode(WIFI_STA); // Setup ESP in client mode
    WiFi.begin(ssid, password); // Connect to wifi

    int wait_passes = 0;
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (++wait_passes >= 10) {
        WiFi.begin(ssid, password);
        wait_passes = 0;
      }
    }

    Serial.println("\nConnected to WiFi!");
    Serial.println("Local IP address: " + WiFi.localIP().toString());
  }

  void VerifyWifi() {
    while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0))
      WiFi.reconnect();
  }

  void SetupOTA() {
    ArduinoOTA.onStart([]() { // Prepare OTA stuff
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.setHostname(rigIdentifier);
    ArduinoOTA.begin();
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

  void parseSerialData(void) {
    miner_id = getValue(inputBuffer, SEP_TOKEN, 0).toInt();
    if (miner_id > MAX_MINERS) {
      // Should never happen, unless some communications conflict...
      // If so, drop the packet because the miner will try again...
      newDataFromSerial = false;
      return;
    }

    Miners[miner_id].Solution = getValue(inputBuffer, SEP_TOKEN, 1).toInt();
  }

  void getInputSerialData(void) {
    if (Serial.available() > 0) {
      char x = Serial.read();

      if (x == END_TOKEN) {
        serialReadInProgress = false;
        newDataFromSerial = true;
        inputBuffer += END_TOKEN;
        parseSerialData();
      }

      if(serialReadInProgress) {
        inputBuffer += x;
      }

      if (x == RECV_TOKEN) {
        serialReadInProgress = true;
      }
    }
  }

  void setupMiner(uint32_t which) {
    if (which >= MAX_MINERS)
      return;

    AsyncClient *aClient = Miners[which].Client;
    if(aClient)//client already exists
      return;

    aClient = new AsyncClient();
    if(!aClient)//could not allocate client
      return;

    aClient->onError([](void * arg, AsyncClient * client, int error){
      Serial.println("Connect Error");
      aClient = NULL;
      delete client;
    }, NULL);

    aClient->onConnect([](void * arg, AsyncClient * client){
      Serial.println("Connected");
      aClient->onError(NULL, NULL);

      client->onDisconnect([](void * arg, AsyncClient * c){
        Serial.println("Disconnected");
        aClient = NULL;
        delete c;
      }, NULL);

      client->onData([](void * arg, AsyncClient * c, void * data, size_t len){
        Serial.print("\r\nData: ");
        Serial.println(len);
        uint8_t * d = (uint8_t*)data;
        for(size_t i=0; i<len;i++)
          Miners[which].inputBUffer += d[i];
      }, NULL);
    }, NULL);

    if(!aClient->connect(host, port)){
      Serial.println("Connect Fail");
      AsyncClient * client = aClient;
      aClient = NULL;
      delete client;
    }

    Miners[which].Client = aClient;
  }

} // namespace

void setup() {
  Serial.begin(9600);

  SetupWifi();
  SetupOTA();

  for (int a=0; a<MAX_MINERS; a++)
    setupMiner(a);
    
}

void loop() {
  VerifyWifi();          // Verify/Connect wifi access
  ArduinoOTA.handle();   // Check/Handle OTA updates
  getInputSerialData();  // Has a miner sent a solution back
}
