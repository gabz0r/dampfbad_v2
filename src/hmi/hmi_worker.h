#ifndef HMI_WORKER_H
#define HMI_WORKER_H

#define SHELLY_LIGHT "http://192.168.178.152/relay/0?turn=on"

#include "wlan/wlan.h"
#include "hmi/hmi.h"

#include<vector>
#include <String>
#include <sstream>
#include <iostream>

class HmiWorker {
public:
    static void proc_LISTWIFI(WlanController *wlan, HmiInterface *hmi);
    static void proc_DISCWIFI(WlanController *wlan, HmiInterface *hmi);
    static void proc_CONNECT(WlanController *wlan, HmiInterface *hmi, std::vector<std::string> cmd);
    static void proc_DISABLESLEEP(HmiInterface *hmi);
    static void proc_KEEPALIVE(HmiInterface *hmi);
    static void proc_WIFISTAT(HmiInterface *hmi);
    static void proc_REQTIME(HmiInterface *hmi);
    static void proc_LIGHT(WlanController *wlan);

    static void ext_CONNECTED(HmiInterface *hmi, bool navMain = true);
    static void ext_CONNECT_ERROR(HmiInterface *hmi, WlanController *wlan);

    static bool isWifiConnecting;
};

#endif