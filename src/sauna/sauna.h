#ifndef SAUNA_H
#define SAUNA_H

#define RS485_RE 4
#define RS485_OE 3

#define START_PACKET_BYTE 0x24
#define END_PACKET_BYTE 0x23

#include <Arduino.h>
#include <esp32-hal-timer.h>
#include <array>
#include <String>
#include <sstream>
#include "hmi/hmi.h"

extern volatile int duration;
void IRAM_ATTR onTimer();
extern portMUX_TYPE saunaMux;

class SaunaController {
public:
    SaunaController();
    bool startSauna(int minues, int degrees, HmiInterface *hmi);
    void stopSauna();


private:
    HardwareSerial *rs485Sauna;
    hw_timer_t *steamTimer;
    std::array<uint8_t, 19> controlPacket;
};

#endif