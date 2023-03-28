#include "Hoymiles.h"

HoymilesClass::HoymilesClass(/* args */)
{
}

HoymilesClass::~HoymilesClass()
{
}

void HoymilesClass::init(const char *Sel_DTU)
{
    Use_DTU = Sel_DTU;

    GetHoymilesStatus();
    delay(1000);
    getHoymilesLimitDTU();
}

void HoymilesClass::loop()
{
}

void HoymilesClass::GetHoymilesStatus()
{
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

                JsonObject inverters_AC_0 = inverters[i]["AC"]["0"];
                HoyACProduction[i] = inverters_AC_0["Power"]["v"];

                /*
                Serial.print("Serial: ");
                Serial.println(HoyInverterSerial[i]);

                Serial.print("Reachable: ");
                Serial.println(HoyAvailable[i]);

                Serial.print("Limit Absolute: ");
                Serial.println(HoyLimitAbsolute[i]);

                Serial.print("Limit Relative: ");
                Serial.println(HoyLimitRelative[i]);

                Serial.print("AC Production: ");
                Serial.println(HoyACProduction[i]);
                */
            }
        }
    }
    http.end();
}

void HoymilesClass::getHoymilesLimitDTU()
{
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
                // HoyLimitRelative[i] = (int)limit_status["limit_relative"];
                // HoyLimitSetStatus[i] = limit_status["limit_set_status"];

                Serial.print("MaxPower: ");
                Serial.println(HoyMaxPower[i]);

                Serial.print("MinPower: ");
                Serial.println(HoyMinPower[i]);
            }
        }
    }
    http.end();
}

void HoymilesClass::SetHoymilesLimitDTU(int pLimit)
{
    char url[100];
    char data[150];

    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        if (HoyInverterSerial[i] != 0)
        {
            delay(SET_LIMIT_DELAY_IN_SECONDS_MULTIPLE_INVERTER);
        }

        int relLimit = pLimit / HoyMaxPower[i] * 100;

        /*
        int Factor = HoyMaxPower[i] / GetMaxWattFromAllInverters();
        int NewLimit = int(pLimit * Factor);
        NewLimit = ZeroExport.ApplyLimitsToSetpointInverter(i, NewLimit);
        */

        if (Use_DTU == "Ahoy")
        {
            snprintf(url, sizeof(url), "http://%s%s", i, "/api/ctrl");
            sprintf(data, "%s%s%s%d%s", "{\"id\": ", i, ", \"cmd\": \"limit_persistent_relative\", \"val\": ", relLimit, "}");

            Serial.print("RelativeLimit Data: ");
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
            sprintf(url, "%s%s%s", "http://", HoyInverterSerial[i], "/api/limit/config");
            sprintf(data, "%s%s%s%d%s", "data={\"serial\":\"", HoyInverterSerial[i], "\", \"limit_type\":1, \"limit_value\":", relLimit, "}");

            http.begin(url);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            http.setAuthorization(DTU_USER, DTU_PASS);
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

int HoymilesClass::GetHoymilesAvailable()
{
    GetHoymilesStatus();
    int GetHoymilesAvailable = 0;

    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        if (HoyAvailable[i] == true)
        {
            GetHoymilesAvailable += 1;
        }
    }
    return GetHoymilesAvailable;
}

int HoymilesClass::GetHoymilesActualPower()
{
    GetHoymilesStatus();
    int ActualPower = 0;

    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        ActualPower += HoyACProduction[i];
    }

    return ActualPower;
}

int HoymilesClass::GetLimitFromAllInverters()
{
    int Limit = 0;
    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        Limit += HoyLimitAbsolute[i];
    }
    return Limit;
}

int HoymilesClass::GetMaxWattFromAllInverters()
{
    int maxWatt = 0;
    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        maxWatt += HoyMaxPower[i];
    }
    return maxWatt;
}

int HoymilesClass::GetMinWattFromAllInverters()
{
    int minWatt = 0;
    for (int i = 0; i < DTU_INVERTER_COUNT; i++)
    {
        minWatt += HoyMinPower[i];
    }
    return minWatt;
}

HoymilesClass Hoymiles;