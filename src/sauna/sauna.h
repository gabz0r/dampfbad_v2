#ifndef SAUNA_H
#define SAUNA_H

#define RS485_RE 4
#define RS485_OE 3

#define TEMP_J3_AMBIENT 5
#define TEMP_J6_STEAM 6
#define TEMP_UPD_INTERVAL_MS 1000
#define REM_MIN_UPD_INTERVAL_MS 29000


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

#include <Thermistor.h>
#include <NTC_Thermistor.h>
#include <SmoothThermistor.h>

#include "hmi/hmi.h"

extern volatile int duration;
void IRAM_ATTR onTimer();
extern portMUX_TYPE saunaMux;

class SaunaController {
public:
    SaunaController();
    bool startSauna(int minues, int degrees, HmiInterface *hmi);
    void process(HmiInterface *hmi);
    void stopSauna(HmiInterface *hmi);


private:
    HardwareSerial *rs485Sauna;
    hw_timer_t *steamTimer;
    std::array<uint8_t, 19> controlPacket;
    SmoothThermistor* thAmbient;
    SmoothThermistor* thSauna;
    unsigned long lastTempUpdate;
    unsigned long lastRemMinUpdate;
    bool heatLocal;
    bool heatRemote;
    int fixedDuration;
    int fixedTemp;
};

#endif