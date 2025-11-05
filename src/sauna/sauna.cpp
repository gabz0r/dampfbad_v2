#include "sauna.h"

volatile int duration = 0;
portMUX_TYPE saunaMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer() {
    Serial.print("TIMER REMAINING");
    portENTER_CRITICAL_ISR(&saunaMux);
    duration--;
    portEXIT_CRITICAL_ISR(&saunaMux);

    Serial.println(duration);
} 


SaunaController::SaunaController() {
    this->rs485Sauna = new HardwareSerial(2);
    this->rs485Sauna->begin(9600, SERIAL_8N1, 2, 1);
    pinMode(RS485_OE, OUTPUT);
    pinMode(RS485_RE, OUTPUT);

    digitalWrite(RS485_OE, LOW); // inactive low
    digitalWrite(RS485_RE, HIGH); // inactive high
    this->controlPacket = std::array<uint8_t, 19> { 
        START_PACKET_BYTE, 
        0x31, 
        0x32, 
        0x4D, 
        0x64, 
        0x6C, 
        0x00, 
        0x66, 
        0x70, 
        0x35, 
        0x35, 
        0x32,
	    0x33, 
        0x35, 
        0x32, 
        0x68, 
        0xC3, 
        0x87, 
        END_PACKET_BYTE
    };

    this->steamTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(this->steamTimer, &onTimer, true);
    timerAlarmWrite(this->steamTimer, 60000000, true);
}

bool SaunaController::startSauna(int minutes, int degrees, HmiInterface *hmi) {
    digitalWrite(RS485_OE, HIGH);
    std::string now = hmi->time();

    duration = minutes;

    std::stringstream ss;
    std::string targetTemp;
    ss << degrees;
    ss >> targetTemp;

    controlPacket[6] = 0x4F; // on, 0x6F = off
    controlPacket[9] = targetTemp[0];
    controlPacket[10] = targetTemp[1];
    controlPacket[11] = now[0];
    controlPacket[12] = now[1];
    controlPacket[13] = now[2];
    controlPacket[14] = now[3];
Serial.println("writing sauna packet");
    digitalWrite(RS485_OE, HIGH);
    rs485Sauna->write(controlPacket.data(), 19);
    rs485Sauna->flush();
    digitalWrite(RS485_OE, LOW);

    timerAlarmEnable(this->steamTimer);

    return true;
}
