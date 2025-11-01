#include "wlan.h"

WlanController::WlanController() {
    this->scanResultCallback = nullptr;
    this->wifiConnectResult = nullptr;
    this->foundNetworks = std::vector<std::string>();
    this->lastScanMillis = millis();
    this->shouldScan = false;
    this->connectStartedMillis = 0;
    this->connectingTo = std::string();
    this->httpClient = new HTTPClient();

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
}

WlanController::~WlanController() {
    delete(this->httpClient);
}

void WlanController::scanNetworksAsync() {
    if (!this->shouldScan) {
        WiFi.mode(WIFI_STA);
        this->shouldScan = true;
        WiFi.mode(WIFI_STA);
        WiFi.scanNetworks(true);
        this->lastScanMillis = millis();
    }
}

void WlanController::attachScanResultCallback(void (*callback)(std::vector<std::string>)) {
    this->scanResultCallback = callback;
}

void WlanController::attachConnectResultCallback(void (*callback)(std::string, bool)) {
    this->wifiConnectResult = callback;
}

void WlanController::process() {
    unsigned long currentMillis = millis();

    if(this->connectStartedMillis != 0 && this->wifiConnectResult) {
        if(currentMillis - this->connectStartedMillis > WLAN_CONNECT_TIMEOUT_MS) {
            this->wifiConnectResult(this->connectingTo, false);
            this->connectStartedMillis = 0;
            this->connectingTo = "";
        }

        if(WiFi.status() == WL_CONNECTED) {
            this->wifiConnectResult(this->connectingTo, true);
            this->connectStartedMillis = 0;
            this->connectingTo = "";
            WiFi.setAutoReconnect(true);
        }
    }

    if(this->shouldScan && currentMillis - lastScanMillis > SCAN_INTERVAL_MS) {
        Serial.println("scan...");
        WiFi.scanNetworks(true);
        lastScanMillis = millis();
    }

    if(this->shouldScan) {
        int found = WiFi.scanComplete();
        if(found > 0) {
            this->foundNetworks.clear();
            shouldScan = false;

            for (int i = 0; i < found; i++)
            {
                this->foundNetworks.push_back(WiFi.SSID(i).c_str());
            }

            std::sort(this->foundNetworks.begin(), this->foundNetworks.end());
            this->foundNetworks.erase(std::unique(this->foundNetworks.begin(), this->foundNetworks.end()), this->foundNetworks.end());

            if(this->scanResultCallback) {
                this->scanResultCallback(this->foundNetworks);
            }
                
            WiFi.scanDelete();
        }
    }
}

void WlanController::connect(std::string ssid, std::string key) {
    WiFi.begin(ssid.c_str(), key.c_str());
    this->connectingTo = ssid;
    this->connectStartedMillis = millis();
    this->shouldScan = false;
}

void WlanController::disconnect() {
    if(WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }
    WiFi.setAutoReconnect(false);
}

std::string WlanController::getRequest(std::string url) {
    if(WiFi.status() == WL_CONNECTED) {
        this->httpClient->begin(url.c_str());
        int httpResponseCode = this->httpClient->GET();
        if(httpResponseCode != 200 && httpResponseCode != 201) {
            return NULL;
        }
        return std::string(this->httpClient->getString().c_str());
    }

    return NULL;
}

bool WlanController::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}