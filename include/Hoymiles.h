#pragma once

#include "Configuration.h"
#include "defaults.h"
#include "Arduino.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "PowerMeter.h"
#include "ZeroExport.h"

class HoymilesClass
{
private:
    const char *Use_DTU;
    HTTPClient http;

    // delay time after sending limit to Hoymiles Inverter when using more than one Inverter
    const uint16_t SET_LIMIT_DELAY_IN_SECONDS_MULTIPLE_INVERTER = 2000;

public:
    HoymilesClass(/* args */);
    ~HoymilesClass();
    void init(const char *Sel_DTU);
    void loop();

    void GetHoymilesStatus();
    void SetHoymilesLimitDTU(int pLimit);
    void getHoymilesLimitDTU();
    int GetMaxWattFromAllInverters();
    int GetMinWattFromAllInverters();
    int GetHoymilesActualPower();
    int GetHoymilesAvailable();
    int GetLimitFromAllInverters();

    int CurrentLimit[DTU_INVERTER_COUNT];
    int HoyMaxPower[DTU_INVERTER_COUNT];
    int HoyMinPower[DTU_INVERTER_COUNT];

    const char *HoyLimitSetStatus[DTU_INVERTER_COUNT];
    const char *HoyInverterSerial[DTU_INVERTER_COUNT];
    bool HoyAvailable[DTU_INVERTER_COUNT];
    int HoyLimitAbsolute[DTU_INVERTER_COUNT];
    int HoyLimitRelative[DTU_INVERTER_COUNT];
    int HoyACProduction[DTU_INVERTER_COUNT];
};

extern HoymilesClass Hoymiles;