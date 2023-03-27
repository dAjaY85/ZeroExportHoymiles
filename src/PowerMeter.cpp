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
    if (setPowerMeter == "MQTT")
    {
        GetPowermeterWatts = Network.MQTT_PowerMeter;
    }
    else if (setPowerMeter == "Shelly3EM")
    {
        GetPowermeterWatts = GetPowermeterWattsShelly3EM();
    }
    else if (setPowerMeter == "Tasmota")
    {
        GetPowermeterWatts = GetPowermeterWattsTasmota();
    }
    else
    {
        Serial.println("Error: no powermeter defined!");
        GetPowermeterWatts = 0;
    }
}

int PowerMeterClass::GetPowermeterWattsTasmota()
{
    /*
    String url = "http://" + String(TASMOTA_IP) + "/cm?cmnd=status%2010";
    ParsedData = http.get(url).json();
    int Watts = ParsedData[TASMOTA_JSON_STATUS][TASMOTA_JSON_PAYLOAD_MQTT_PREFIX][TASMOTA_JSON_POWER_MQTT_LABEL].toInt();
    Serial.print("powermeter: ");
    Serial.print(Watts);
    Serial.println(" Watt");
    return Watts;
    */
}

int PowerMeterClass::GetPowermeterWattsShelly3EM()
{
    /*
    String url = "http://" + String(SHELLY_IP) + "/status";
    ParsedData = http.get(url).json();
    int Watts = ParsedData['total_power'].toInt();
    Serial.print("powermeter: ");
    Serial.print(Watts);
    Serial.println(" Watt");
    return Watts;
    */
}

PowerMeterClass PowerMeter;