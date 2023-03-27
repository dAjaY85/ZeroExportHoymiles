#include "ZeroExport.h"

ZeroExportClass::ZeroExportClass(/* args */)
{
}

ZeroExportClass::~ZeroExportClass()
{
}

void ZeroExportClass::init(const char *Sel_DTU)
{
    Use_DTU = Sel_DTU;

    GetHoymilesStatus();
    delay(1000);
    getLimitDTU();

    SLOW_APPROX_LIMIT = GetMaxWattFromAllInverters() * SLOW_APPROX_LIMIT_IN_PERCENT / 100;

    newLimitSetpoint = GetMaxWattFromAllInverters();
    if (GetHoymilesAvailable())
    {
        SetLimitDTU(newLimitSetpoint);
    }
    delay(SET_LIMIT_DELAY_IN_SECONDS);
}

void ZeroExportClass::loop()
{
    if ((millis() - lastTime) > LOOP_INTERVAL_IN_SECONDS)
    {
        GetHoymilesStatus();

        int PreviousLimitSetpoint = newLimitSetpoint;
        if (GetHoymilesAvailable())
        {
            int powermeterWatts = 0;
            for (int x = 0; x < int(LOOP_INTERVAL_IN_SECONDS / POLL_INTERVAL_IN_SECONDS); x++)
            {
                powermeterWatts = PowerMeter.GetPowermeterWatts;
                if (powermeterWatts > POWERMETER_MAX_POINT)
                {
                    if (JUMP_TO_MAX_LIMIT_ON_GRID_USAGE)
                    {
                        newLimitSetpoint = GetMaxWattFromAllInverters();
                    }
                    else
                    {
                        newLimitSetpoint = PreviousLimitSetpoint + powermeterWatts - POWERMETER_TARGET_POINT;
                    }
                    newLimitSetpoint = ApplyLimitsToSetpoint(newLimitSetpoint);
                    SetLimitDTU(newLimitSetpoint);
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
                if (PreviousLimitSetpoint >= GetMaxWattFromAllInverters())
                {
                    int hoymilesActualPower = GetHoymilesActualPower();
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
                if (PreviousLimitSetpoint < GetMaxWattFromAllInverters())
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
                SetLimitDTU(newLimitSetpoint);
            }
        }
        lastTime = millis();
    }
}

int ZeroExportClass::CutLimitToProduction(int pSetpoint)
{
    if (pSetpoint != GetMaxWattFromAllInverters())
    {
        int ActualPower = GetHoymilesActualPower();
        // prevent the setpoint from running away...
        if (pSetpoint > ActualPower + (GetMaxWattFromAllInverters() * MAX_DIFFERENCE_BETWEEN_LIMIT_AND_OUTPUTPOWER / 100))
        {
            pSetpoint = int(ActualPower + (GetMaxWattFromAllInverters() * MAX_DIFFERENCE_BETWEEN_LIMIT_AND_OUTPUTPOWER / 100));
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
    if (pSetpoint > HoyMaxPower[pInverter])
    {
        pSetpoint = HoyMaxPower[pInverter];
    }
    if (pSetpoint < HoyMinPower[pInverter])
    {
        pSetpoint = HoyMinPower[pInverter];
    }
    return pSetpoint;
}

bool ZeroExportClass::GetHoymilesAvailable()
{
    GetHoymilesStatus();
    bool GetHoymilesAvailable = false;

    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        if (HoyAvailable[i] == true)
        {
            GetHoymilesAvailable = true;
            break;
        }
    }
    return GetHoymilesAvailable;
}

float ZeroExportClass::GetHoymilesActualPower()
{
    GetHoymilesStatus();
    float ActualPower = 0;

    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        ActualPower += HoyACProduction[i];
    }

    return ActualPower;
}

void ZeroExportClass::GetHoymilesStatus()
{
    HTTPClient http;
    char url[100] = "http://";
    strcat(url, DTU_IP);
    if (Use_DTU == "Ahoy")
    {
        strcat(url, "/api/index");
        Serial.println(url);
        http.begin(url);

        if (http.GET() == 200)
        {
            // Parsing
            DynamicJsonDocument doc(DTU_INVERTER_COUNT * 2048);
            DeserializationError error = deserializeJson(doc, http.getString());
            if (error)
            {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return;
            }
            JsonArray inverters = doc["inverters"];
            // Parameters
            for (int i = 0; i < DTU_INVERTER_COUNT; i++)
            {
                HoyAvailable[i] = inverters[i]["is_avail"];

                // HoyInverterSerial[i] = inverters[i]["serial"];
                // HoyLimitAbsolute[i] = inverters[i]["limit_absolute"];
                // HoyACProduction[i] = inverters[i]["AC"][0]["Power"]["v"];

                Serial.print("Reachable: ");
                Serial.println(HoyAvailable[i]);
            }
        }
    }
    else if (Use_DTU == "OpenDTU")
    {
        strcat(url, "/api/livedata/status");
        Serial.println(url);
        http.begin(url);

        if (http.GET() == 200)
        {
            // Parsing
            DynamicJsonDocument doc(DTU_INVERTER_COUNT * 2048);
            DeserializationError error = deserializeJson(doc, http.getString());
            if (error)
            {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return;
            }
            JsonArray inverters = doc["inverters"];
            // Parameters
            for (int i = 0; i < DTU_INVERTER_COUNT; i++)
            {
                HoyInverterSerial[i] = inverters[i]["serial"];
                HoyAvailable[i] = inverters[i]["reachable"];
                HoyLimitAbsolute[i] = inverters[i]["limit_absolute"];
                HoyLimitRelative[i] = inverters[i]["limit_relative"];
                (HoyLimitAbsolute[i] * DTU_MIN_POWER_PERCENT / 100);

                for (JsonPair inverters_AC_0_item : inverters[i]["AC"]["0"].as<JsonObject>())
                {
                    const char *inverters_0_AC_0_item_key = inverters_AC_0_item.key().c_str(); // "Power", "Voltage", ...

                    if (inverters_0_AC_0_item_key == "Power")
                    {
                        HoyACProduction[i] = inverters_AC_0_item.value()["v"];
                    }
                }

                Serial.print("Serial: ");
                Serial.println(HoyInverterSerial[i]);

                Serial.print("Reachable: ");
                Serial.println(HoyAvailable[i]);

                Serial.print("Limit Absolute: ");
                Serial.println(HoyLimitAbsolute[i]);

                Serial.print("AC Production: ");
                Serial.println(HoyACProduction[i]);
            }
        }
    }
    http.end();
}

void ZeroExportClass::getLimitDTU()
{
    HTTPClient http;
    char url[100] = "http://";
    strcat(url, DTU_IP);
    if (Use_DTU == "Ahoy")
    {
        // GetHoymilesAvailable[i] = GetHoymilesAvailableAhoy(i);
    }
    else if (Use_DTU == "OpenDTU")
    {
        strcat(url, "/api/limit/status");
        Serial.println(url);
        http.begin(url);

        if (http.GET() == 200)
        {
            // Parsing
            DynamicJsonDocument doc(DTU_INVERTER_COUNT * 192);
            DeserializationError error = deserializeJson(doc, http.getString());
            if (error)
            {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
                return;
            }

            for (int i = 0; i < DTU_INVERTER_COUNT; i++)
            {
                JsonObject limit_status = doc[HoyInverterSerial[i]];

                HoyMaxPower[i] = limit_status["max_power"];
                HoyMinPower[i] = ((HoyMaxPower[i] * DTU_MIN_POWER_PERCENT) / 100);
                HoyLimitRelative[i] = limit_status["limit_relative"];
                HoyLimitSetStatus[i] = limit_status["limit_set_status"];

                Serial.print("MaxPower: ");
                Serial.println(HoyMaxPower[i]);

                Serial.print("MinPower: ");
                Serial.println(HoyMinPower[i]);

                Serial.print("LimitRelative: ");
                Serial.println(HoyLimitRelative[i]);

                Serial.print("LimitSetStatus: ");
                Serial.println(HoyLimitSetStatus[i]);
            }
        }
    }
    http.end();
}

void ZeroExportClass::SetLimitDTU(int pLimit)
{
    HTTPClient http;
    char url[100];
    char data[150];

    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        if (HoyInverterSerial[i] != 0)
        {
            delay(SET_LIMIT_DELAY_IN_SECONDS_MULTIPLE_INVERTER);
        }

        int relLimit = pLimit / HoyMaxPower[i] * 100;
        strcat(url, DTU_IP);

        float Factor = HoyMaxPower[i] / GetMaxWattFromAllInverters();
        int NewLimit = int(pLimit * Factor);
        NewLimit = ApplyLimitsToSetpointInverter(i, NewLimit);
        if (Use_DTU == "Ahoy")
        {
            snprintf(url, sizeof(url), "http://%s%s", i, "/api/ctrl");
            sprintf(data, "%s%s%s%d%s", "{\"id\": ", i, ", \"cmd\": \"limit_persistent_relative\", \"val\": ", relLimit, "}");

            Serial.print("RelativeLimit: ");
            Serial.println(data);

            http.begin(url);
            http.addHeader("Content-type", "application/json");
            http.addHeader("Accept", "text/plain");
            int httpCode = http.POST(data);
            if (httpCode > 0)
            {
                String response = http.getString();

                Serial.println(httpCode);
                Serial.println(response);
            }
        }
        else if (Use_DTU == "OpenDTU")
        {
            snprintf(url, sizeof(url), "http://%s%s", HoyInverterSerial[i], "/api/limit/config");
            sprintf(data, "%s%s%s%d%s", "data={\"serial\":\"", HoyInverterSerial[i], "\", \"limit_type\":1, \"limit_value\":", relLimit, "}");

            Serial.print("RelativeLimit: ");
            Serial.println(data);

            http.begin(url);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            int httpCode = http.POST(data);
            if (httpCode > 0)
            {
                String response = http.getString();

                Serial.println(httpCode);
                Serial.println(response);
            }
        }
        else
        {
            Serial.println("Error: DTU Type not defined");
        }
        http.end();
        CurrentLimit[i] = pLimit;
    }
}

int ZeroExportClass::GetMaxWattFromAllInverters()
{
    int maxWatt = 0;
    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        maxWatt += HoyMaxPower[i];
    }
    return maxWatt;
}

int ZeroExportClass::GetMinWattFromAllInverters()
{
    int minWatt = 0;
    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        minWatt += HoyMinPower[i];
    }
    return minWatt;
}

int ZeroExportClass::ApplyLimitsToSetpoint(int pSetpoint)
{
    if (pSetpoint > GetMaxWattFromAllInverters())
    {
        pSetpoint = GetMaxWattFromAllInverters();
    }
    if (pSetpoint < GetMinWattFromAllInverters())
    {
        pSetpoint = GetMinWattFromAllInverters();
    }
    return pSetpoint;
}

ZeroExportClass ZeroExport;