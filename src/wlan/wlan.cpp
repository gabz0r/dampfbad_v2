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
    this->preferences = new Preferences();

    this->preferences->begin("dampfbad", false);
    std::string ssid(this->preferences->getString("ssid", "nval").c_str());
    std::string key(this->preferences->getString("key").c_str());
    this->preferences->end();

    WiFi.mode(WIFI_STA);

    if(ssid != "nval") {
        WebSerial.println(ssid.c_str());
        WebSerial.println(key.c_str());
        this->connect(ssid, key);
    } else {
        WiFi.disconnect();
    }
}

WlanController::~WlanController() {
    delete(this->httpClient);
    delete(this->preferences);
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

            this->preferences->begin("dampfbad", false);
            this->preferences->clear();
            this->preferences->end();
        }

        if(WiFi.status() == WL_CONNECTED) {
            this->wifiConnectResult(this->connectingTo, true);
            this->connectStartedMillis = 0;
            this->connectingTo = "";
            WiFi.setAutoReconnect(true);
        }
    }

    if(this->shouldScan && currentMillis - lastScanMillis > SCAN_INTERVAL_MS) {
        WebSerial.println("scan...");
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
    WebSerial.printf("Connecting to %s with key %s", ssid.c_str(), key.c_str());
    WiFi.begin(ssid.c_str(), key.c_str());
    this->connectingTo = ssid;
    this->connectStartedMillis = millis();
    this->shouldScan = false;

    this->preferences->begin("dampfbad", false);
    this->preferences->clear();
    this->preferences->putString("ssid", ssid.c_str());
    this->preferences->putString("key", key.c_str());
    this->preferences->end();
}

void WlanController::disconnect() {
    if(WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }

    this->preferences->begin("dampfbad", false);
    this->preferences->clear();
    this->preferences->end();

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