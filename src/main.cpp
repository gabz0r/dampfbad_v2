#include <Arduino.h>
#include <HardwareSerial.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <ElegantOTA.h>

#include <vector>
#include <String>

#include "wlan/wlan.h"
#include "hmi/hmi.h"
#include "hmi/hmi_worker.h"
#include "sauna/sauna.h"

WlanController *wlan;
HmiInterface *hmi;
SaunaController *sauna;

AsyncWebServer server(80);


void onNetworkScanResult(std::vector<std::string> networks);
void onNetworkConnectResult(std::string ssid, bool success);

void onHmiCommand(std::vector<std::string> cmd);

void processSerial();

void initOTA();
void onOTAStart();
void onOTAProgress(size_t current, size_t final);
void onOTAEnd(bool success);

void setup() {
  Serial.begin(9600);

  delay(1000);
  WebSerial.println("Hello world");
  
  wlan = new WlanController();
  wlan->attachScanResultCallback(onNetworkScanResult);
  wlan->attachConnectResultCallback(onNetworkConnectResult);

  hmi = new HmiInterface();
  hmi->attachCommandCallback(onHmiCommand);
  delay(100);
  hmi->restart();

  sauna = new SaunaController();
}

uint8_t hmi_buf[16];

void loop() {

  wlan->process();
  hmi->process();
  sauna->process(hmi);

  processSerial();

  ElegantOTA.loop();
  WebSerial.loop();
}

void onNetworkScanResult(std::vector<std::string> networks) {
  hmi->setAvailableNetworks(networks);
}

void onNetworkConnectResult(std::string ssid, bool success) {
  if(success) {
    WebSerial.printf("connected to %s\n", ssid.c_str());
    HmiWorker::ext_CONNECTED(hmi);
    initOTA();
    WebSerial.begin(&server);

  } else {
    WebSerial.println("connect failed");
    HmiWorker::ext_CONNECT_ERROR(hmi, wlan);
  }
}

void onHmiCommand(std::vector<std::string> cmd) {

  WebSerial.print("Executing ");
  for(std::string c : cmd) {
    WebSerial.printf("%s,", c.c_str());
  }

  if(cmd.at(0) == "LISTWIFI") {
    HmiWorker::proc_LISTWIFI(wlan, hmi);
  }
  else if(cmd.at(0) == "DISCWIFI") {
    HmiWorker::proc_DISCWIFI(wlan, hmi);
  }
  else if(cmd.at(0) == "WIFISTAT") {
    HmiWorker::proc_WIFISTAT(hmi);
  } 
  else if(cmd.at(0) == ("CONNECT")) {
    HmiWorker::proc_CONNECT(wlan, hmi, cmd);
  }
  else if(cmd.at(0) == "DISABLESLEEP") {
    HmiWorker::proc_DISABLESLEEP(hmi);
  }
  else if(cmd.at(0) == "REQTIME") {
    HmiWorker::proc_REQTIME(hmi);
  }
  else if(cmd.at(0) == "LIGHT") {
    HmiWorker::proc_LIGHT(wlan);
  } else if(cmd.at(0) == "VENT") {
    HmiWorker::proc_VENT(wlan);
  }
  else if(cmd.at(0) == "HEAT") {
    HmiWorker::proc_HEAT(hmi, sauna, cmd);
  } else if(cmd.at(0) == "STOPHEAT") {
    HmiWorker::proc_STOPHEAT(hmi, sauna);
  }
}

void processSerial() {
  if(Serial.available()) {
    String cmd = Serial.readString();
    if(cmd == "reset") {
      ESP.restart();
    }
  }
}

void initOTA() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  ElegantOTA.begin(&server);

  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);

  server.begin();
  WebSerial.println("HTTP server started");
}

void onOTAStart() {
}

void onOTAProgress(size_t current, size_t final) {
}

void onOTAEnd(bool success) {
}