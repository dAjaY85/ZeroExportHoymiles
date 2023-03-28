#include "PowerMeter.h"

PowerMeterClass::PowerMeterClass()
{
    // constructor implementation
}

PowerMeterClass::~PowerMeterClass()
{
    // destructor implementation
}

void PowerMeterClass::init(const char *sel_Meter)
{
    setPowerMeter = sel_Meter;
}

void PowerMeterClass::loop()
{
    int MeasurementsToAverage = 5;

    if ((millis() - lastTime) > 1000)
    {
        for (int i = 0; i < MeasurementsToAverage; ++i)
        {
            if (setPowerMeter == "MQTT")
            {
                GetPowermeterWatts += Network.MQTT_PowerMeter;
            }
            else if (setPowerMeter == "Shelly3EM")
            {
                GetPowermeterWatts += GetPowermeterWattsShelly3EM();
            }
            else if (setPowerMeter == "Tasmota")
            {
                GetPowermeterWatts += GetPowermeterWattsTasmota();
            }
            else
            {
                Serial.println("Error: no powermeter defined!");
                GetPowermeterWatts = 0;
            }
        }
        GetPowermeterWatts /= MeasurementsToAverage;
        lastTime = millis();
    }
}

int PowerMeterClass::GetPowermeterWattsTasmota()
{
    HTTPClient http;
    char url[100] = "http://";
    strcat(url, POWERMETER_IP);
    strcat(url, "/cm?cmnd=status%2010");
    http.begin(url);

    if (http.GET() == 200)
    {
        // Parsing
        DynamicJsonDocument doc(384);

        DeserializationError error = deserializeJson(doc, http.getString());
        if (error)
        {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return 0;
        }

        JsonObject Tasmota_ENERGY = doc["StatusSNS"]["ENERGY"];
        int Tasmota_Power = Tasmota_ENERGY["Power"]; // 0
        return Tasmota_Power;

        /*
        String url = "http://" + String(TASMOTA_IP) + "/cm?cmnd=status%2010";
        ParsedData = http.get(url).json();
        int Watts = ParsedData[TASMOTA_JSON_STATUS][TASMOTA_JSON_PAYLOAD_MQTT_PREFIX][TASMOTA_JSON_POWER_MQTT_LABEL].toInt();
        return Watts;
        */
    }
    http.end();
    return 0;
}

int PowerMeterClass::GetPowermeterWattsShelly3EM()
{
    HTTPClient http;
    char url[100] = "http://";
    strcat(url, POWERMETER_IP);
    strcat(url, "/status");
    http.begin(url);

    if (http.GET() == 200)
    {
        // Parsing
        DynamicJsonDocument doc(2048);

        DeserializationError error = deserializeJson(doc, http.getString());
        if (error)
        {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
            return 0;
        }

        float total_power = doc["total_power"];
        int Shelly_Power = (int)(total_power + .5);
        return Shelly_Power;

        /*
        String url = "http://" + String(SHELLY_IP) + "/status";
        ParsedData = http.get(url).json();
        int Watts = ParsedData['total_power'].toInt();
        return Watts;
        */
    }
    http.end();
    return 0;
}

PowerMeterClass PowerMeter;