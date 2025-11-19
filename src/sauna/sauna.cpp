#include "sauna.h"

void printBytesHex(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (data[i] < 0x10)
            WebSerial.print('0'); // leading zero
        WebSerial.print(data[i], HEX);
        if (i < len - 1)
            WebSerial.print(' '); // space between bytes
    }
}

SaunaController::SaunaController(HmiInterface *hmi, WlanController *wlan)
{
    this->hmi = hmi;
    this->wlan = wlan;
    this->lightAndVentMinuteOfDay = 0;

    this->rs485Sauna = new HardwareSerial(2);
    this->rs485Sauna->begin(4800, SERIAL_8N1, 2, 1);
    pinMode(RS485_OE, OUTPUT);
    pinMode(RS485_RE, OUTPUT);

    digitalWrite(RS485_OE, LOW); // inactive low
    digitalWrite(RS485_RE, LOW); // active low
    this->controlPacket = std::array<uint8_t, 19>{
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
        END_PACKET_BYTE};

    this->lastTempUpdate = millis();
    this->lastRemMinUpdate = millis();
    this->startedHour = 0;
    this->startedMinute = 0;
    this->endMinute = 0;
    this->endHour = 0;

    this->thAmbient = new SmoothThermistor(new NTC_Thermistor_ESP32(
                                               TEMP_J3_AMBIENT,
                                               REFERENCE_RESISTANCE,
                                               NOMINAL_RESISTANCE_AMBIENT,
                                               NOMINAL_TEMPERATURE,
                                               B_VALUE,
                                               ESP32_ADC_VREF_MV,
                                               ESP32_ANALOG_RESOLUTION),
                                           SMOOTHING_FACTOR);

    this->thSauna = new SmoothThermistor(new NTC_Thermistor_ESP32(
                                             TEMP_J6_STEAM,
                                             REFERENCE_RESISTANCE,
                                             NOMINAL_RESISTANCE_SAUNA,
                                             NOMINAL_TEMPERATURE,
                                             B_VALUE,
                                             ESP32_ADC_VREF_MV,
                                             ESP32_ANALOG_RESOLUTION),
                                         SMOOTHING_FACTOR);

    this->heatLocal = false;
    this->heatRemote = false;
}

bool SaunaController::startSauna(std::string minutes, std::string degrees)
{
    this->heatLocal = true;

    std::string now = this->hmi->time();

    this->fixedDuration = minutes;
    this->fixedTemp = degrees;

    // safe parse "now" expected as "HHMM"
    int sh = 0, sm = 0, dur = 0;
    if (now.size() >= 4 && isdigit((unsigned char)now[0]) && isdigit((unsigned char)now[1]) &&
        isdigit((unsigned char)now[2]) && isdigit((unsigned char)now[3]) && 
        isdigit((unsigned char)minutes[0]) && isdigit((unsigned char)minutes[1]))
    {
        char hourStr[3] = {now[0], now[1], '\0'};
        char minuteStr[3] = {now[2], now[3], '\0'};
        dur = atoi(minutes.c_str());
        sh = atoi(hourStr);
        sm = atoi(minuteStr);
    }
    this->startedHour = sh;
    this->startedMinute = sm;

    int totalStartMin = sh * 60 + sm;
    int totalEndMin = (totalStartMin + dur) % (24 * 60);
    this->endHour = totalEndMin / 60;
    this->endMinute = totalEndMin % 60;
    this->startedMinuteOfDay = totalStartMin;
    this->endMinuteOfDay = totalEndMin;

    WebSerial.print("Setting target temp to ");
    WebSerial.print(degrees.c_str());
    WebSerial.println();
    WebSerial.print(degrees[0]);
    WebSerial.print(" ");
    WebSerial.print(degrees[1]);
    WebSerial.flush();

    controlPacket[6] = 0x4F; // on, 0x6F = off
    controlPacket[9] = degrees[0];
    controlPacket[10] = degrees[1];
    controlPacket[11] = now[0];
    controlPacket[12] = now[1];
    controlPacket[13] = now[2];
    controlPacket[14] = now[3];

    digitalWrite(RS485_RE, HIGH);
    digitalWrite(RS485_OE, HIGH);
    printBytesHex(controlPacket.data(), 19);
    rs485Sauna->write(controlPacket.data(), 19);
    rs485Sauna->flush();
    digitalWrite(RS485_OE, LOW);
    digitalWrite(RS485_RE, LOW);


    return true;
}

void SaunaController::process()
{
    if (millis() - this->lastTempUpdate > TEMP_UPD_INTERVAL_MS)
    {
        this->lastTempUpdate = millis();
        double tAmbientInC = this->thAmbient->readCelsius() - CALIBRATION_OFFSET_C;
        double tSaunaC = this->thSauna->readCelsius() - CALIBRATION_OFFSET_C;
        hmi->updateTemps(tAmbientInC, tSaunaC);
    }

    if (millis() - this->lastRemMinUpdate > REM_MIN_UPD_INTERVAL_MS && this->heatLocal && this->heatRemote)
    {
        this->lastRemMinUpdate = millis();
        this->wlan->getRequest(SHELLY_VENT_OFF); // keep off while running

        std::string now = this->hmi->time();

        // safe parse "now" expected as "HHMM"
        int ch = 0, cm = 0;
        if (now.size() >= 4 && isdigit((unsigned char)now[0]) && isdigit((unsigned char)now[1]) &&
            isdigit((unsigned char)now[2]) && isdigit((unsigned char)now[3]))
        {
            char hourStr[3] = {now[0], now[1], '\0'};
            char minuteStr[3] = {now[2], now[3], '\0'};
            ch = atoi(hourStr);
            cm = atoi(minuteStr);
        }

        this->currentMinuteOfDay = ch * 60 + cm;

        int remaining = this->endMinuteOfDay - this->currentMinuteOfDay;
        this->hmi->setRemainingMinutes(remaining);

        if(remaining % 5 == 0) {
            this->hmi->tone(500);
        }
        if(remaining <= 0) {
            this->stopSauna();
            this->hmi->tone(2000);
            this->hmi->updateHeatButton(false);
            this->lightAndVentMinuteOfDay = this->endMinuteOfDay + 5;
        }
    }

    if(millis() - this->lastRemMinUpdate > REM_MIN_UPD_INTERVAL_MS && !this->heatLocal && !this->heatRemote && this->lightAndVentMinuteOfDay != 0) {
        HmiWorker::proc_VENT(this->wlan);
        HmiWorker::proc_LIGHT(this->wlan);
        this->startedMinuteOfDay = 0;
        this->startedMinute = 0;
        this->startedHour = 0;
        this->currentMinuteOfDay = 0;
        this->lightAndVentMinuteOfDay = 0;
        this->endMinuteOfDay = 0;
        this->endMinute = 0;
        this->endHour = 0;
    }

    unsigned char acknowledgePacket[14] = {};

    while (this->rs485Sauna->available())
    {
        this->rs485Sauna->readBytesUntil(END_PACKET_BYTE, acknowledgePacket, 14);
        acknowledgePacket[13] = END_PACKET_BYTE;

        WebSerial.print("Ack = (s->c) ");
        Serial.write(acknowledgePacket, 14);
        WebSerial.println();

        if (this->heatLocal && !this->heatRemote)
        {
            this->heatRemote = true;
            this->hmi->updateHeatButton(true);
        }
        else if (!this->heatLocal && this->heatRemote)
        {
            this->hmi->updateHeatButton(false);
            this->hmi->setRemainingMinutes(25);
        }
    }
}

void SaunaController::stopSauna()
{
    this->heatLocal = false;

    std::string now = this->hmi->time();

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
    digitalWrite(RS485_OE, HIGH);
    printBytesHex(controlPacket.data(), 19);

    rs485Sauna->write(controlPacket.data(), 19);
    rs485Sauna->flush();
    digitalWrite(RS485_OE, LOW);
    digitalWrite(RS485_RE, LOW);
}