#pragma once

#include "arduino.h"
#include "defaults.h"
#include "Network.h"

class PowerMeterClass
{
private:
    int GetPowermeterWattsMQTT();
    int GetPowermeterWattsTasmota();
    int GetPowermeterWattsShelly3EM();
    

    const char *setPowerMeter;

public:
    PowerMeterClass(/* args */);
    ~PowerMeterClass();
    void init(const char *sel_Meter);
    void loop();
    int GetPowermeterWatts;
    
};

extern PowerMeterClass PowerMeter;
