#ifndef WLAN_H
#define WLAN_H

#define SCAN_INTERVAL_MS 5000
#define WLAN_CONNECT_TIMEOUT_MS 10000

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include <String>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebSerial.h>

class WlanController {
public:
    WlanController();
    ~WlanController();
    void scanNetworksAsync();
    void attachScanResultCallback(void (*callback)(std::vector<std::string>));
    void attachConnectResultCallback(void (*callback)(std::string, bool));
    void connect(std::string ssid, std::string key);
    void disconnect();
    void process();
    std::string getRequest(std::string url);
    static bool isConnected();

private:
    void (*scanResultCallback)(std::vector<std::string>);
    void (*wifiConnectResult)(std::string, bool);
    bool shouldScan;
    std::vector<std::string> foundNetworks;
    unsigned long lastScanMillis;
    unsigned long connectStartedMillis;
    std::string connectingTo;
    HTTPClient *httpClient;
    Preferences *preferences;
};

#endif