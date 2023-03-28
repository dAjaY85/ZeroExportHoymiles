#pragma once

#include "defaults.h"
#include "Arduino.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "PowerMeter.h"
#include "Hoymiles.h"

class ZeroExportClass
{
private:
    // max difference in percent between SetpointLimit change to approximate the power to new setpoint
    const uint8_t SLOW_APPROX_LIMIT_IN_PERCENT = 20;
    // interval time for setting limit to Hoymiles
    const uint16_t LOOP_INTERVAL_IN_SECONDS = 20000;
    // delay time after sending limit to Hoymiles Inverter
    const uint16_t SET_LIMIT_DELAY_IN_SECONDS = 5000;
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

    uint32_t newLimitSetpoint;
    uint32_t lastTime;

    int CutLimitToProduction(int pSetpoint);
    int ApplyLimitsToSetpoint(int pSetpoint);

public:
    ZeroExportClass();
    ~ZeroExportClass();

    int ApplyLimitsToSetpointInverter(int pInverter, int pSetpoint);

    void init();
    void loop();
};

extern ZeroExportClass ZeroExport;