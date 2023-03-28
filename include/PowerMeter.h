#pragma once

#include "arduino.h"
#include "defaults.h"
#include "Network.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>

class PowerMeterClass
{
private:
    int GetPowermeterWattsMQTT();
    int GetPowermeterWattsTasmota();
    int GetPowermeterWattsShelly3EM();

    const char *setPowerMeter;

    uint32_t lastTime = 0;

public:
    PowerMeterClass(/* args */);
    ~PowerMeterClass();
    void init(const char *sel_Meter);
    void loop();
    int GetPowermeterWatts;
};

extern PowerMeterClass PowerMeter;
