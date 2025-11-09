#include <Arduino.h>
#include <HardwareSerial.h>

#include<vector>
#include <String>

#include "wlan/wlan.h"
#include "hmi/hmi.h"
#include "hmi/hmi_worker.h"
#include "sauna/sauna.h"

WlanController *wlan;
HmiInterface *hmi;
SaunaController *sauna;

void onNetworkScanResult(std::vector<std::string> networks);
void onNetworkConnectResult(std::string ssid, bool success);

void onHmiCommand(std::vector<std::string> cmd);

void processSerial();

void setup() {
  Serial.begin(9600);

  delay(1000);
  Serial.println("Hello world");
  
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

  delay(100);
}

void onNetworkScanResult(std::vector<std::string> networks) {
  hmi->setAvailableNetworks(networks);
}

void onNetworkConnectResult(std::string ssid, bool success) {
  if(success) {
    Serial.printf("connected to %s\n", ssid.c_str());
    HmiWorker::ext_CONNECTED(hmi);
  } else {
    Serial.println("connect failed");
    HmiWorker::ext_CONNECT_ERROR(hmi, wlan);
  }
}

void onHmiCommand(std::vector<std::string> cmd) {
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
  } else if(cmd.at(0) == "HEAT") {
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