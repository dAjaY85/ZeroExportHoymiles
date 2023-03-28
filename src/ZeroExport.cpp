#include "ZeroExport.h"

ZeroExportClass::ZeroExportClass(/* args */)
{
}

ZeroExportClass::~ZeroExportClass()
{
}

void ZeroExportClass::init()
{
    SLOW_APPROX_LIMIT = Hoymiles.GetMaxWattFromAllInverters() * SLOW_APPROX_LIMIT_IN_PERCENT / 100;

    newLimitSetpoint = Hoymiles.GetMaxWattFromAllInverters();

    if (Hoymiles.GetHoymilesAvailable())
    {
        Hoymiles.SetHoymilesLimitDTU(newLimitSetpoint);
    }
    delay(SET_LIMIT_DELAY_IN_SECONDS);
}

void ZeroExportClass::loop()
{
    if ((millis() - lastTime) > LOOP_INTERVAL_IN_SECONDS)
    {
        Hoymiles.GetHoymilesStatus();

        int PreviousLimitSetpoint = newLimitSetpoint;
        if (Hoymiles.GetHoymilesAvailable())
        {
            int powermeterWatts = 0;
            for (int x = 0; x < int(LOOP_INTERVAL_IN_SECONDS / POLL_INTERVAL_IN_SECONDS); x++)
            {
                powermeterWatts = PowerMeter.GetPowermeterWatts;

                Serial.print("PowerMeterWatts: ");
                Serial.println(powermeterWatts);

                if (powermeterWatts > POWERMETER_MAX_POINT)
                {
                    if (JUMP_TO_MAX_LIMIT_ON_GRID_USAGE)
                    {
                        newLimitSetpoint = Hoymiles.GetMaxWattFromAllInverters();
                    }
                    else
                    {
                        newLimitSetpoint = PreviousLimitSetpoint + powermeterWatts - POWERMETER_TARGET_POINT;
                    }
                    newLimitSetpoint = ApplyLimitsToSetpoint(newLimitSetpoint);
                    Hoymiles.SetHoymilesLimitDTU(newLimitSetpoint);
                    if (int(LOOP_INTERVAL_IN_SECONDS) - SET_LIMIT_DELAY_IN_SECONDS - x * POLL_INTERVAL_IN_SECONDS <= 0)
                    {
                        break;
                    }
                    else
                    {
                        delay(int(LOOP_INTERVAL_IN_SECONDS) - SET_LIMIT_DELAY_IN_SECONDS - x * POLL_INTERVAL_IN_SECONDS);
                    }
                    break;
                }
                else
                {
                    delay(POLL_INTERVAL_IN_SECONDS);
                }
            }

            if (MAX_DIFFERENCE_BETWEEN_LIMIT_AND_OUTPUTPOWER != 100)
            {
                int CutLimit = CutLimitToProduction(newLimitSetpoint);
                if (CutLimit != newLimitSetpoint)
                {
                    newLimitSetpoint = CutLimit;
                    PreviousLimitSetpoint = newLimitSetpoint;
                }
            }

            if (powermeterWatts > POWERMETER_MAX_POINT)
            {
                return;
            }

            // producing too much power: reduce limit
            if (powermeterWatts < (POWERMETER_TARGET_POINT - POWERMETER_TOLERANCE))
            {
                if (PreviousLimitSetpoint >= Hoymiles.GetMaxWattFromAllInverters())
                {
                    int hoymilesActualPower = Hoymiles.GetHoymilesActualPower();
                    newLimitSetpoint = hoymilesActualPower + powermeterWatts - POWERMETER_TARGET_POINT;
                    int LimitDifference = (PreviousLimitSetpoint - newLimitSetpoint);
                    newLimitSetpoint = newLimitSetpoint + (LimitDifference / 4);
                    if (newLimitSetpoint > hoymilesActualPower)
                    {
                        newLimitSetpoint = hoymilesActualPower;
                    }
                    Serial.println("overproducing: reduce limit based on actual power");
                }
                else
                {
                    newLimitSetpoint = PreviousLimitSetpoint + powermeterWatts - POWERMETER_TARGET_POINT;
                    // check if it is necessary to approximate to the setpoint with some more passes. this reduce overshoot
                    int LimitDifference = (PreviousLimitSetpoint - newLimitSetpoint);
                    if (LimitDifference > SLOW_APPROX_LIMIT)
                    {
                        Serial.println("overproducing: reduce limit based on previous limit setpoint by approximation");
                        newLimitSetpoint = newLimitSetpoint + (LimitDifference / 4);
                    }
                    else
                    {
                        Serial.println("overproducing: reduce limit based on previous limit setpoint");
                    }
                }
            }

            // producing too little power: increase limit
            else if (powermeterWatts > (POWERMETER_TARGET_POINT + POWERMETER_TOLERANCE))
            {
                if (PreviousLimitSetpoint < Hoymiles.GetMaxWattFromAllInverters())
                {
                    newLimitSetpoint = PreviousLimitSetpoint + powermeterWatts - POWERMETER_TARGET_POINT;
                    Serial.println("Not enough energy producing: increasing limit");
                }
                else
                {
                    Serial.println("Not enough energy producing: limit already at maximum");
                }
            }

            // check for upper and lower limits
            newLimitSetpoint = ApplyLimitsToSetpoint(newLimitSetpoint);
            // set new limit to inverter
            if (newLimitSetpoint != PreviousLimitSetpoint)
            {
                Hoymiles.SetHoymilesLimitDTU(newLimitSetpoint);
            }
        }
        lastTime = millis();
    }
}

int ZeroExportClass::CutLimitToProduction(int pSetpoint)
{
    if (pSetpoint != Hoymiles.GetMaxWattFromAllInverters())
    {
        int ActualPower = Hoymiles.GetHoymilesActualPower();
        // prevent the setpoint from running away...
        if (pSetpoint > ActualPower + (Hoymiles.GetMaxWattFromAllInverters() * MAX_DIFFERENCE_BETWEEN_LIMIT_AND_OUTPUTPOWER / 100))
        {
            pSetpoint = int(ActualPower + (Hoymiles.GetMaxWattFromAllInverters() * MAX_DIFFERENCE_BETWEEN_LIMIT_AND_OUTPUTPOWER / 100));
            Serial.print("Cut limit to ");
            Serial.print(pSetpoint);
            Serial.print(" Watt, limit was higher than ");
            Serial.print(MAX_DIFFERENCE_BETWEEN_LIMIT_AND_OUTPUTPOWER);
            Serial.println(" percent of live-production");
        }
    }
    return int(pSetpoint);
}

int ZeroExportClass::ApplyLimitsToSetpointInverter(int pInverter, int pSetpoint)
{
    if (pSetpoint > Hoymiles.HoyMaxPower[pInverter])
    {
        pSetpoint = Hoymiles.HoyMaxPower[pInverter];
    }
    if (pSetpoint < Hoymiles.HoyMinPower[pInverter])
    {
        pSetpoint = Hoymiles.HoyMinPower[pInverter];
    }
    return pSetpoint;
}

int ZeroExportClass::ApplyLimitsToSetpoint(int pSetpoint)
{
    if (pSetpoint > Hoymiles.GetMaxWattFromAllInverters())
    {
        pSetpoint = Hoymiles.GetMaxWattFromAllInverters();
    }
    if (pSetpoint < Hoymiles.GetMinWattFromAllInverters())
    {
        pSetpoint = Hoymiles.GetMinWattFromAllInverters();
    }
    return pSetpoint;
}

ZeroExportClass ZeroExport;