#pragma once

#include "defaults.h"
#include "Arduino.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include "PowerMeter.h"

class ZeroExportClass
{
private:
    // max difference in percent between SetpointLimit change to approximate the power to new setpoint
    const uint8_t SLOW_APPROX_LIMIT_IN_PERCENT = 20;
    // interval time for setting limit to Hoymiles
    const uint16_t LOOP_INTERVAL_IN_SECONDS = 20000;
    // delay time after sending limit to Hoymiles Inverter
    const uint16_t SET_LIMIT_DELAY_IN_SECONDS = 5000;
    // delay time after sending limit to Hoymiles Inverter when using more than one Inverter
    const uint16_t SET_LIMIT_DELAY_IN_SECONDS_MULTIPLE_INVERTER = 2000;
    // polling interval for powermeter(must be < LOOP_INTERVAL_IN_SECONDS)
    uint16_t POLL_INTERVAL_IN_SECONDS = 1000;
    // when powermeter> 0 : (True) : always jump to maxLimit of inverter; (False) : increase limit based on previous limit
    bool JUMP_TO_MAX_LIMIT_ON_GRID_USAGE = true;
    // max difference between Limit and real output power in % of HOY_MAX_WATT(100 = disabled)
    uint8_t MAX_DIFFERENCE_BETWEEN_LIMIT_AND_OUTPUTPOWER = 100;

    // -- - global defines for control behaviour -- -
    // this is the target power for powermeter in watts
    int8_t POWERMETER_TARGET_POINT = -75;
    // this is the tolerance(pos and neg) around the target point.in this range no adjustment will be set
    uint8_t POWERMETER_TOLERANCE = 25;
    // this is the max power to regulate the limit. if your powermeter is above this point, the limit jumps to 100 % (when JUMP_TO_MAX_LIMIT_ON_GRID_USAGE is set to TRUE).
    // Must be higher than POWERMETER_TARGET_POINT + POWERMETER_TOLERANCE
    int8_t POWERMETER_MAX_POINT;

    int16_t SLOW_APPROX_LIMIT;

    int CurrentLimit[DTU_INVERTER_COUNT];
    int HoyMaxPower[DTU_INVERTER_COUNT];
    int HoyMinPower[DTU_INVERTER_COUNT];

    const char *HoyLimitSetStatus[DTU_INVERTER_COUNT];
    const char *HoyInverterSerial[DTU_INVERTER_COUNT];
    bool HoyAvailable[DTU_INVERTER_COUNT];
    float HoyLimitAbsolute[DTU_INVERTER_COUNT];
    float HoyLimitRelative[DTU_INVERTER_COUNT];
    float HoyACProduction[DTU_INVERTER_COUNT];

    const char *Use_DTU;
    uint32_t newLimitSetpoint;
    uint32_t lastTime;

    void GetHoymilesStatus();
    void SetLimitDTU(int pLimit);
    void getLimitDTU();
    int GetMaxWattFromAllInverters();
    int GetMinWattFromAllInverters();
    int ApplyLimitsToSetpoint(int pSetpoint);
    int ApplyLimitsToSetpointInverter(int pInverter, int pSetpoint);
    void AverageFilter(bool NewValueAvailable, float NewValue, uint8_t Positions);
    int CutLimitToProduction(int pSetpoint);
    float GetHoymilesActualPower();
    bool GetHoymilesAvailable();

public:
    ZeroExportClass();
    ~ZeroExportClass();

    void init(const char *Sel_DTU);
    void loop();
};

extern ZeroExportClass ZeroExport;