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

#include "version-info.h"   // Created by calling increase-version.bat; requires
                            // increase-version.dat file which stores the current
                            // build number - Needed for VersionInfo() macro.
#include "wifi-manager.hpp" // contains #defines NETWORK_SSID / NETWORK_PWD and
                            // DUCO_USERNAME so my personal information isn't
                            // pushed to git repo since this file is .gitignore'd
#include "getvalue.h"

#define LED_BUILTIN             1

#define BLINK_SHARE_FOUND       1
#define BLINK_SETUP_COMPLETE    2
#define BLINK_CLIENT_CONNECT    3
#define BLINK_RESET_DEVICE      5

#define MAX_MINERS              1
#define isValidMinerId(x) (x<=MAX_MINERS)

namespace /* anonymous */ {
  const char* ssid          = NETWORK_SSID;    // Change this to your WiFi SSID
  const char* password      = NETWORK_PWD;     // Change this to your WiFi password
  const char* ducouser      = DOCU_USERNAME;   // Change this to your Duino-Coin username
  const char* rigIdentifier = "ATTiny85";      // Change this if you want a custom miner name

  const char * host = "51.15.127.80"; // Static server IP
  const int port = 2811;
  unsigned int Shares = 0; // Share variable

  const char SEND_TOKEN = '>';
  const char RECV_TOKEN = '<';
  const char END_TOKEN = '\n';
  const char SEP_TOKEN = ',';

  // Serial input data
  String serialBuffer = "";
  boolean serialReadInProgress = false;
  boolean newDataFromSerial = false;

  // DUCO_Miner Miners[MAX_MINERS];

  void setupWifi() {
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

  void verifyWifi() {
    while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0))
      WiFi.reconnect();
  }

  void setupOTA() {
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

  void blink(uint8_t count, uint8_t pin = LED_BUILTIN) {
    uint8_t state = HIGH;

    for (int x=0; x<(count << 1); ++x) {
      digitalWrite(pin, state ^= HIGH);
      delay(50);
    }
  }

  void sendJobToMiner(const int which) {
    String data = "45db4ef2a7a5ce029e4d12065ab80d10e44ce07a,9f0c10a7a16b84f54e148e7c7c4e3a9b591f624f,6";
    Serial.print(String(SEND_TOKEN)+String(which)+String(SEP_TOKEN)+data+String(END_TOKEN));
    blink(BLINK_CLIENT_CONNECT);
  }

  void parseSerialData(void) {
    uint32_t miner_id = getValue(serialBuffer, SEP_TOKEN, 0).toInt();
    if (!isValidMinerId(miner_id)) {
      // Should never happen, unless some communications conflict...
      // If so, drop the packet because the miner will try again...
      newDataFromSerial = false;
      return;
    }

    // sendJobToMiner(miner_id);

    serialBuffer = "";
    newDataFromSerial = false;
  }

  void handleInputSerialData(void) {
    if (Serial.available() > 0) {
      char x = Serial.read();

      if (x == END_TOKEN) {
        serialReadInProgress = false;
        newDataFromSerial = true;
        serialBuffer += END_TOKEN;
        parseSerialData();
        blink(BLINK_SHARE_FOUND);
      }

      if(serialReadInProgress) {
        serialBuffer += x;
      }

      if (x == RECV_TOKEN) {
        serialReadInProgress = true;
      }
    }
  }

} // namespace

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  setupWifi();
  setupOTA();

  sendJobToMiner(0);

  blink(BLINK_SETUP_COMPLETE); // Blink 2 times - indicate sucessfull connection with wifi network
}

void loop() {
  verifyWifi();             // Verify/Connect wifi access
  ArduinoOTA.handle();      // Check/Handle OTA updates
  handleInputSerialData();  // Has a miner sent a solution back
}
