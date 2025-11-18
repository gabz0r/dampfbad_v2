#include "sauna.h"

volatile int duration = 0;
portMUX_TYPE saunaMux = portMUX_INITIALIZER_UNLOCKED;

volatile bool beepProgress = false;
volatile bool beepFinished = false;

void printBytesHex(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) WebSerial.print('0'); // leading zero
        WebSerial.print(data[i], HEX);
        if (i < len - 1) WebSerial.print(' '); // space between bytes
    }
}

void IRAM_ATTR onTimer() {
    portENTER_CRITICAL_ISR(&saunaMux);
    duration--;
    if(duration % 5 == 0) {
        beepProgress = true;
    } else if(duration == 0) {
        beepFinished = true;
    }
    portEXIT_CRITICAL_ISR(&saunaMux);
} 


SaunaController::SaunaController() {
    this->rs485Sauna = new HardwareSerial(2);
    this->rs485Sauna->begin(4800, SERIAL_8N1, 2, 1);
    pinMode(RS485_OE, OUTPUT);
    pinMode(RS485_RE, OUTPUT);

    digitalWrite(RS485_OE, LOW); // inactive low
    digitalWrite(RS485_RE, LOW); // active low
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

    this->lastTempUpdate = millis();
    this->lastRemMinUpdate = millis();

    this->thAmbient = new SmoothThermistor(new NTC_Thermistor_ESP32(
        TEMP_J3_AMBIENT,
        REFERENCE_RESISTANCE,
        NOMINAL_RESISTANCE_AMBIENT,
        NOMINAL_TEMPERATURE,
        B_VALUE,
        ESP32_ADC_VREF_MV,
        ESP32_ANALOG_RESOLUTION
    ), SMOOTHING_FACTOR);

    this->thSauna = new SmoothThermistor(new NTC_Thermistor_ESP32(
        TEMP_J6_STEAM,
        REFERENCE_RESISTANCE,
        NOMINAL_RESISTANCE_AMBIENT,
        NOMINAL_TEMPERATURE,
        B_VALUE,
        ESP32_ADC_VREF_MV,
        ESP32_ANALOG_RESOLUTION
    ), SMOOTHING_FACTOR);

    this->heatLocal = false;
    this->heatRemote = false;
}

bool SaunaController::startSauna(int minutes, int degrees, HmiInterface *hmi) {
    std::string now = hmi->time();

    this->fixedDuration = minutes;
    this->fixedTemp = degrees;

    duration = minutes;

    std::stringstream ss;
    std::string targetTemp;
    ss << degrees;
    ss >> targetTemp;

    WebSerial.printf("Setting target temp to %s C\n", targetTemp);
    WebSerial.flush();
    WebSerial.printf("Char representation: %s %s", targetTemp[0], targetTemp[1]);

    controlPacket[6] = 0x4F; // on, 0x6F = off
    controlPacket[9] = targetTemp[0];
    controlPacket[10] = targetTemp[1];
    controlPacket[11] = now[0];
    controlPacket[12] = now[1];
    controlPacket[13] = now[2];
    controlPacket[14] = now[3];

    digitalWrite(RS485_RE, HIGH);
    delay(10);
    digitalWrite(RS485_OE, HIGH);
    printBytesHex(controlPacket.data(), 19);
    rs485Sauna->write(controlPacket.data(), 19);
    rs485Sauna->flush();
    digitalWrite(RS485_OE, LOW);
    delay(10);
    digitalWrite(RS485_RE, LOW);

    this->heatLocal = true;

    return true;
}

void SaunaController::process(HmiInterface *hmi) {
    if (millis() - this->lastTempUpdate > TEMP_UPD_INTERVAL_MS) {
        this->lastTempUpdate = millis();
        double tAmbientInC = this->thAmbient->readCelsius() - CALIBRATION_OFFSET_C;
        double tSaunaC = this->thSauna->readCelsius() - CALIBRATION_OFFSET_C;
        hmi->updateTemps(tAmbientInC, tSaunaC);
    }

    if (millis() - this->lastRemMinUpdate > REM_MIN_UPD_INTERVAL_MS && this->heatLocal && this->heatRemote) {
        this->lastRemMinUpdate = millis();
        hmi->setRemainingMinutes(duration);
    }

    if(this->heatLocal && this->heatRemote && duration <= 0) {
        this->stopSauna(hmi);
    }
    
    unsigned char acknowledgePacket[14] = {};

    while(this->rs485Sauna->available()) {
        this->rs485Sauna->readBytesUntil(END_PACKET_BYTE, acknowledgePacket, 14);
        acknowledgePacket[13] = END_PACKET_BYTE;


        WebSerial.print("Ack = (s->c) ");
        Serial.write(acknowledgePacket, 14);
        WebSerial.println();

        if(this->heatLocal && !this->heatRemote) {
            this->heatRemote = true;
            hmi->updateHeatButton(true);
            timerAlarmEnable(this->steamTimer);
        }
        else if(!this->heatLocal && this->heatRemote) {
            hmi->updateHeatButton(false);
            hmi->setRemainingMinutes(25);
            timerAlarmDisable(this->steamTimer);
        }
    }
}

void SaunaController::stopSauna(HmiInterface *hmi) {
    std::string now = hmi->time();

    std::stringstream ss;
    std::string targetTemp;
    ss << this->fixedTemp;
    ss >> targetTemp;

    controlPacket[6] = 0x6F; // of, 0x4F = on
    controlPacket[9] = targetTemp[0];
    controlPacket[10] = targetTemp[1];
    controlPacket[11] = now[0];
    controlPacket[12] = now[1];
    controlPacket[13] = now[2];
    controlPacket[14] = now[3];

    digitalWrite(RS485_RE, HIGH);
    delay(10);
    digitalWrite(RS485_OE, HIGH);
    printBytesHex(controlPacket.data(), 19);

    rs485Sauna->write(controlPacket.data(), 19);
    rs485Sauna->flush();
    digitalWrite(RS485_OE, LOW);    
    delay(10);
    digitalWrite(RS485_RE, LOW);

    this->heatLocal = false;
}