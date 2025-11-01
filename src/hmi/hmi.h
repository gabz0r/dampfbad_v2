#ifndef HMI_H
#define HMI_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <String>
#include <sstream>
#include <vector>

#include "time.h"
#include "wlan/wlan.h"

#define VALID_COMMANDS "REQTIME,KEEPALIVE,HEAT,STOPHEAT,LIGHT,VENT,LISTWIFI,DISCWIFI,WIFISTAT,CONNECT,DISABLESLEEP"

#define NTP "de.pool.ntp.org"
#define TZ "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"


class HmiInterface {
public:
    HmiInterface();
    ~HmiInterface();
    void sendCommand(std::string cmdStr);
    void attachCommandCallback(void (*callback)(std::vector<std::string>));
    void process();

    void setAvailableNetworks(std::vector<std::string> networks);
    void setWifiStatusConnecting(std::string withSSID);
    void setWifiConnectionStatus(bool connected, bool navMain = true);
    void gotoSleep();
    void updateRtc();
    void setMaySleep(bool maySleep);
    void restart();

private:
    HardwareSerial *hmiSerial;
    void (*pollCallback)(std::vector<std::string>);
    char buffer[512];
    unsigned long lastRtcUpdate;
    unsigned long lastInteraction;
    bool maySleep;
    bool isSleeping;
};


#endif