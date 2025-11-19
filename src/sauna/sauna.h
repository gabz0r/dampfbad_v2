#ifndef SAUNA_H
#define SAUNA_H

#define RS485_RE 4
#define RS485_OE 3

#define TEMP_J3_AMBIENT 5
#define TEMP_J6_STEAM 6
#define TEMP_UPD_INTERVAL_MS 1000
#define REM_MIN_UPD_INTERVAL_MS 15000
#define SHELLY_VENT_OFF "http://192.168.178.152/relay/0?turn=off"


#define START_PACKET_BYTE 0x24
#define END_PACKET_BYTE 0x23

#define REFERENCE_RESISTANCE       10000
#define NOMINAL_RESISTANCE_AMBIENT 10300
#define NOMINAL_RESISTANCE_SAUNA   10150
#define NOMINAL_TEMPERATURE        25
#define B_VALUE                    3950
#define ESP32_ANALOG_RESOLUTION    4095
#define ESP32_ADC_VREF_MV          3300

#define SMOOTHING_FACTOR 5
#define CALIBRATION_OFFSET_C 4


#include <Arduino.h>
#include <esp32-hal-timer.h>
#include <array>
#include <String>
#include <sstream>

#include <WebSerial.h>

#include "hmi/hmi_worker.h"

#include "thermistor/Thermistor.h"
#include "thermistor/NTC_Thermistor.h"
#include "thermistor/SmoothThermistor.h"

#include "hmi/hmi.h"

class SaunaController {
public:
    SaunaController(HmiInterface *hmi, WlanController *wlan);
    bool startSauna(std::string minutes, std::string degrees);
    void process();
    void stopSauna();


private:
    HardwareSerial *rs485Sauna;
    std::array<uint8_t, 19> controlPacket;
    SmoothThermistor* thAmbient;
    SmoothThermistor* thSauna;
    unsigned long lastTempUpdate;
    unsigned long lastRemMinUpdate;
    bool heatLocal;
    bool heatRemote;
    std::string fixedDuration;
    std::string fixedTemp;
    int startedHour;
    int startedMinute;
    int startedMinuteOfDay;
    int currentMinuteOfDay;
    int endHour;
    int endMinute;
    int endMinuteOfDay;
    int lightAndVentMinuteOfDay;
    HmiInterface *hmi;
    WlanController *wlan;
};

#endif