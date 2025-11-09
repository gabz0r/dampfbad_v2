#include "hmi_worker.h"
bool HmiWorker::isWifiConnecting = false;


void HmiWorker::proc_LISTWIFI(WlanController *wlan, HmiInterface *hmi) {
    if(HmiWorker::isWifiConnecting) return;

    wlan->scanNetworksAsync();
    hmi->sendCommand("dataWifiResult.clear()");
    hmi->sendCommand("dataWifiResult.insert(\"Suchen...\")");
}

void HmiWorker::proc_DISCWIFI(WlanController *wlan, HmiInterface *hmi) {
    Serial.println("DISCWIFI");
    wlan->disconnect();
    hmi->setWifiConnectionStatus(false);
}

void HmiWorker::proc_CONNECT(WlanController *wlan, HmiInterface *hmi, std::vector<std::string> cmd) {
    Serial.println("CONNECT");
    hmi->sendCommand("input.txt=\"\"");
    hmi->sendCommand("show.txt=\"\"");

    if(cmd.size() != 3) {
        hmi->setWifiConnectionStatus(false);
        return;
    }

    std::string ssid = cmd.at(1);
    std::string key = cmd.at(2);
    
    // as the hmi cursor is just a cycled append of | character, we need to potentially remove it

    std::size_t start_pos = key.find("|", start_pos);
    Serial.print("index of | =");
    Serial.println(start_pos);
    if(start_pos != std::string::npos) {
        Serial.print("remove ");
        Serial.println(key.back());
        key.pop_back();
    }

    Serial.print("key is now ");
    Serial.println(key.c_str());

    wlan->connect(ssid, key);
    hmi->setWifiStatusConnecting(ssid);
    HmiWorker::isWifiConnecting = true;
}

void HmiWorker::proc_WIFISTAT(HmiInterface *hmi) {
    if(WlanController::isConnected()) {
        HmiWorker::ext_CONNECTED(hmi, false);
    }
}

void HmiWorker::proc_REQTIME(HmiInterface *hmi) {
    hmi->updateRtc();
}

void HmiWorker::proc_LIGHT(WlanController *wlan) {
    wlan->getRequest(SHELLY_LIGHT);
}

void HmiWorker::proc_HEAT(HmiInterface *hmi, SaunaController *sauna, std::vector<std::string> params) {
    int tempC = atoi(params.at(1).c_str());
    int durationM = atoi(params.at(2).c_str());

    sauna->startSauna(tempC, durationM, hmi);
}

void HmiWorker::proc_STOPHEAT(HmiInterface *hmi, SaunaController *sauna) {
    sauna->stopSauna(hmi);
}



void HmiWorker::ext_CONNECTED(HmiInterface *hmi, bool navMain) {
    Serial.println("CONNECTED");
    hmi->setMaySleep(true);

    hmi->setWifiConnectionStatus(true, navMain);
    HmiWorker::isWifiConnecting = false;
}

void HmiWorker::ext_CONNECT_ERROR(HmiInterface *hmi, WlanController *wlan) {
    Serial.println("CONNECT ERROR");

    hmi->setWifiConnectionStatus(false);
    wlan->disconnect();
    HmiWorker::isWifiConnecting = false;
}

void HmiWorker::proc_DISABLESLEEP(HmiInterface *hmi) {
    hmi->setMaySleep(false);
}