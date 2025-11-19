#include "hmi.h"

HmiInterface::HmiInterface() {
    this->hmiSerial = new HardwareSerial(1);
    this->hmiSerial->begin(9600, SERIAL_8N1, 18, 17);
    this->pollCallback = nullptr;
    this->lastRtcUpdate = millis();
    this->lastInteraction = millis();
    this->isSleeping = false;
    this->maySleep = true;
    this->currentPage = PAGE_MAIN;
    this->toneStart = 0;
    this->toneDuration = 0;
    pinMode(PIEZO, OUTPUT);
}

HmiInterface::~HmiInterface() {
    delete(this->hmiSerial);
}

void HmiInterface::sendCommand(std::string cmdStr) {
    WebSerial.println(cmdStr.c_str());
    this->hmiSerial->write(cmdStr.c_str());
    this->hmiSerial->write(0xFF);
    this->hmiSerial->write(0xFF);
    this->hmiSerial->write(0xFF);
}

void HmiInterface::attachCommandCallback(void (*callback)(std::vector<std::string>)) {
    this->pollCallback = callback;
}

void HmiInterface::process() {
    if(millis() - lastInteraction > 120000) gotoSleep();
    if(millis() - lastRtcUpdate > 30000) updateRtc();

    this->checkToneEnd();

    uint16_t available = this->hmiSerial->available();
    if(available > 0 && this->pollCallback) {
        String cmd = hmiSerial->readStringUntil(';');
        WebSerial.print("CMD=");
        WebSerial.println(cmd);
        WebSerial.flush();
        delay(200);

        //verify and extract command
        std::vector<std::string> splits;
        std::istringstream sstream(std::string(cmd.c_str()));
        std::string p;

        while (std::getline(sstream, p, ':')) {
            splits.push_back(p);
            WebSerial.print("pushing ");
            std::string dbg("\"" + p + "\"");
            WebSerial.println(dbg.c_str());
        }

        int startingIndex = 0;
        std::string validCommands(VALID_COMMANDS);

        for(std::string token : splits) {
            if(validCommands.find(splits.at(startingIndex)) != std::string::npos && splits.at(startingIndex) != "") {
                break;
            }
            startingIndex++;
        }
        if(startingIndex == splits.size()) return;

        WebSerial.print("starting index is now = ");
        WebSerial.println(startingIndex);

        if(startingIndex > 0) { 
            splits.erase(splits.begin(), splits.begin() + startingIndex );
            WebSerial.print("new first element");
            WebSerial.println(splits.at(0).c_str());
        }

        if(splits.size() > 0 && validCommands.find(splits.at(0)) != std::string::npos) {
            WebSerial.print("Valid: ");
            WebSerial.println(splits.at(0).c_str());

            if(splits.at(0) == "PAGEMAIN") {
                this->currentPage = PAGE_MAIN;
            } else if(splits.at(0) == "PAGEWIFI") {
                this->currentPage = PAGE_WIFI;
            } else if(splits.at(0) == "PAGEKEYBD") {
                this->currentPage = PAGE_KEYBD;
            } else if(splits.at(0) == "PAGECLOCK") {
                this->currentPage = PAGE_CLOCK;
            } else {
                this->pollCallback(splits);
            }

            this->lastInteraction = millis();
            this->isSleeping = false;
        }
    }
}

void HmiInterface::setAvailableNetworks(std::vector<std::string> networks) {
    if(this->currentPage != PAGE_WIFI) return;

    this->sendCommand("dataWifiResult.clear()");
    for(std::string net : networks) {
        std::ostringstream insert;
        insert << "dataWifiResult.insert(\"" << net << "\")";
        this->sendCommand(insert.str());
    }
}

void HmiInterface::setWifiStatusConnecting(std::string withSSID) {
    if(this->currentPage != PAGE_WIFI) return;

    this->sendCommand("dataWifiResult.clear()");
    std::ostringstream insert;
    insert << "dataWifiResult.insert(\"Verbinde zu " << withSSID << "\")";
    this->sendCommand(insert.str());
}

void HmiInterface::setWifiConnectionStatus(bool connected, bool navMain) {
    if(connected) {
        if(navMain) {
            this->sendCommand("page main");
            delay(1000);
        }

        this->sendCommand("btnWifiPage.picc=4");
    }
    else {
        if(this->currentPage == PAGE_WIFI) {
            this->sendCommand("dataWifiResult.clear()");
            this->sendCommand("dataWifiResult.insert(\"Verbindungsfehler!\")");
            this->sendCommand("page main");
            delay(1000);
        }

        this->sendCommand("btnWifiPage.picc=3");
    }
}

void HmiInterface::gotoSleep() {
    if(!isSleeping && maySleep) {
        if(atoi(this->internalTime.c_str()) > 0 && 
           atoi(this->internalTime.c_str()) < 630) {
            this->sendCommand("dim=10");
        } else {
            this->sendCommand("dim=50");
        }

        this->sendCommand("page clock");
        this->isSleeping = true;
    }
}

void HmiInterface::updateRtc() {

    this->lastRtcUpdate = millis();
    
    if(!WlanController::isConnected()) {
        WebSerial.println("Cannot obtain time, disconnected!");
        return;
    }

    struct tm timeinfo;
    configTzTime(TZ, NTP); 

    if(!getLocalTime(&timeinfo)){
        WebSerial.println("Failed to obtain time!");
        return;
    }

    // allocate 3 bytes: two digits + terminating '\0'
    char hour[3];
    char minute[3];
    // use safe snprintf and proper integer format
    snprintf(hour, sizeof(hour), "%02d", timeinfo.tm_hour);
    snprintf(minute, sizeof(minute), "%02d", timeinfo.tm_min);


    std::ostringstream insertRtc3;
    insertRtc3 << "rtc3=" << hour;
    this->sendCommand(insertRtc3.str());

    std::ostringstream insertRtc4;
    insertRtc4 << "rtc4=" << minute;
    this->sendCommand(insertRtc4.str());


    std::ostringstream internalTime;
    internalTime << hour[0] << hour[1] << minute[0] << minute[1];
    this->internalTime = internalTime.str();

    WebSerial.print("internal time is ");
    WebSerial.println(this->internalTime.c_str());

    if(this->internalTime == "0300") {
        WebSerial.println("ciao, daily restart...");
        ESP.restart();
    }

    if(this->currentPage == PAGE_MAIN) {
        std::ostringstream clockMini;
        clockMini << "txtClockMini.txt=\"" << hour << ":" << minute << "\"";
        this->sendCommand(clockMini.str());
    } else if(this->currentPage == PAGE_CLOCK) {
        std::ostringstream directH10;
        directH10 << "txtTimeH10.txt=\"" << hour[0] << "\"";
        this->sendCommand(directH10.str());

        std::ostringstream directH1;
        directH1 << "txtTimeH1.txt=\"" << hour[1] << "\"";
        this->sendCommand(directH1.str());

        std::ostringstream directM10;
        directM10 << "txtTimeM10.txt=\"" << minute[0] << "\"";
        this->sendCommand(directM10.str());

        std::ostringstream directM1;
        directM1 << "txtTimeM1.txt=\"" << minute[1] << "\"";
        this->sendCommand(directM1.str());
    }
}

void HmiInterface::setMaySleep(bool maySleep) {
    this->maySleep = maySleep;
}

void HmiInterface::restart() {
    this->sendCommand("page main");
}

std::string HmiInterface::time() {
    return this->internalTime;
}

void HmiInterface::updateTemps(double ambient, double sauna) {
    if(this->currentPage == PAGE_MAIN) {

        char bufA[8];
        char bufS[8];

        snprintf(bufA, sizeof(bufA), "%.1f", ambient);
        snprintf(bufS, sizeof(bufS), "%.1f", sauna);

        for (int i = 0; bufA[i] != '\0'; ++i) {
            if (bufA[i] == '.') bufA[i] = ',';
        }
        for (int i = 0; bufS[i] != '\0'; ++i) {
            if (bufS[i] == '.') bufS[i] = ',';
        }

        std::stringstream s_cmdA;
        s_cmdA << "txtRoomTemp.txt=\"" << bufA << "\"";
        this->sendCommand(s_cmdA.str());

        std::stringstream s_cmdS;
        s_cmdS << "txtShowerTemp.txt=\"" << bufS << "\"";
        this->sendCommand(s_cmdS.str());
    }
}

void HmiInterface::updateHeatButton(bool active) {
    if(active) {
        this->sendCommand("btnSteamStart.picc=2");
    } else {
        this->sendCommand("btnSteamStart.picc=0");
    }
}

void HmiInterface::setRemainingMinutes(int remaining) {
    std::stringstream s_remaining;
    s_remaining << "txtDuration.txt=\"" << remaining << "\"";
    this->sendCommand(s_remaining.str());
}

void HmiInterface::tone(unsigned long duration_ms) {
    this->toneStart = millis();
    this->toneDuration = duration_ms;
    digitalWrite(PIEZO, HIGH);
}

void HmiInterface::checkToneEnd() {
    if(this->toneStart != 0 && millis() - this->toneStart > this->toneDuration) {
        digitalWrite(PIEZO, LOW);
        this->toneStart = 0;
        this->toneDuration = 0;
    }
}