#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "defaults.h"

// SELECT DTU
bool USE_AHOY = false;
bool USE_OPENDTU = true;
// SELECT POWERMETER
bool USE_TASMOTA;
bool USE_SHELLY_3EM;
// TASMOTA
const char *TASMOTA_IP;
const char *TASMOTA_JSON_STATUS;
const char *TASMOTA_JSON_PAYLOAD_MQTT_PREFIX;
const char *TASMOTA_JSON_POWER_MQTT_LABEL;
// SHELLY 3EM
const char *SHELLY_IP;
// COMMON
const int INVERTER_COUNT = 2;
const int LOOP_INTERVAL_IN_SECONDS = 20000;
const int LIMIT_DELAY_IN_SECONDS_MULTIPLE_INVERTER = 2000;
const int LIMIT_DELAY_IN_SECONDS = 5000;

const int POLL_INTERVAL_IN_SECONDS = 1;
const int POWERMETER_TARGET_POINT = -75;
const int POWERMETER_TOLERANCE = 25;
const int POWERMETER_MAX_POINT = 0;
bool JUMP_TO_MAX_LIMIT_ON_GRID_USAGE;

int CurrentLimit[INVERTER_COUNT];
int HoyMaxPower[INVERTER_COUNT];
int HoyMinPower[INVERTER_COUNT] = {50, 100};

const char *HoyLimitSetStatus[INVERTER_COUNT];
const char *HoyInverterSerial[INVERTER_COUNT];
bool HoyAvailable[INVERTER_COUNT];
float HoyLimitAbsolute[INVERTER_COUNT];
float HoyLimitRelative[INVERTER_COUNT];
float HoyACProduction[INVERTER_COUNT];

uint32_t lastTime;

/*
# grid power
#    ...
#     |
#     | -------> jump limit to HOY_MAX_WATT if (JUMP_TO_MAX_LIMIT_ON_GRID_USAGE = TRUE), else: increasing limit <-------
#     |
#   [0W]      [POWERMETER_MAX_POINT]
#     |
#     | -------> increasing limit <-------
#     |
#  [-50W]     [POWERMETER_TARGET_POINT + POWERMETER_TOLERANCE]
#     |
#     | -------> no limit change between -100W ... -50W <-------
#     |
#  [-75W]     [POWERMETER_TARGET_POINT]
#     |
#     | -------> no limit change between -100W ... -50W <-------
#     |
#  [-100W]    [POWERMETER_TARGET_POINT - POWERMETER_TOLERANCE]
#     |
#     | -------> decreasing limit <-------
#     |
#    ...
*/

int GetPowermeterWattsTasmota()
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

int GetPowermeterWattsShelly3EM()
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

int GetPowermeterWatts()
{
  if (USE_SHELLY_3EM)
  {
    return GetPowermeterWattsShelly3EM();
  }
  else if (USE_TASMOTA)
  {
    return GetPowermeterWattsTasmota();
  }
  else
  {
    Serial.println("Error: no powermeter defined!");
    return 0;
  }
}

int GetMaxWattFromAllInverters()
{
  int maxWatt = 0;
  for (int i = 0; i < INVERTER_COUNT; i++)
  {
    maxWatt += HoyMaxPower[i];
  }
  return maxWatt;
}

int GetMinWattFromAllInverters()
{
  int minWatt = 0;
  for (int i = 0; i < INVERTER_COUNT; i++)
  {
    minWatt += HoyMinPower[i];
  }
  return minWatt;
}

int ApplyLimitsToSetpoint(int pSetpoint)
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

int ApplyLimitsToSetpointInverter(int pInverter, int pSetpoint)
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

void getLimitOpenDTU()
{
  HTTPClient http;
  char url[100] = "http://";
  strcat(url, DTU_IP);
  if (USE_AHOY)
  {
    // GetHoymilesAvailable[i] = GetHoymilesAvailableAhoy(i);
  }
  else if (USE_OPENDTU)
  {
    strcat(url, "/api/limit/status");
    Serial.println(url);
    http.begin(url);

    if (http.GET() == 200)
    {
      // Parsing
      DynamicJsonDocument doc(INVERTER_COUNT * 192);
      DeserializationError error = deserializeJson(doc, http.getString());
      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      for (int i = 0; i < INVERTER_COUNT; i++)
      {
        JsonObject limit_status = doc[HoyInverterSerial[i]];

        HoyMaxPower[i] = limit_status["max_power"];
        HoyLimitRelative[i] = limit_status["limit_relative"];
        HoyLimitSetStatus[i] = limit_status["limit_set_status"];

        Serial.print("MaxPower: ");
        Serial.println(HoyMaxPower[i]);

        Serial.print("LimitRelative: ");
        Serial.println(HoyLimitRelative[i]);

        Serial.print("LimitSetStatus: ");
        Serial.println(HoyLimitSetStatus[i]);
      }
    }
  }
  http.end();
}

void SetLimitAhoy(int pInverterID, int pLimit)
{
  if (HoyInverterSerial[pInverterID] != 0)
  {
    delay(LIMIT_DELAY_IN_SECONDS_MULTIPLE_INVERTER);
  }

  HTTPClient http;
  int relLimit = pLimit / HoyMaxPower[pInverterID] * 100;
  char url[100] = "http://";
  strcat(url, DTU_IP);
  strcat(url, "/api/ctrl");

  char data[150];
  sprintf(data, "%s%s%s%d%s", "{\"id\": ", pInverterID, ", \"cmd\": \"limit_nonpersistent_absolute\", \"val\": ", relLimit, "}");

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
  http.end();
  CurrentLimit[pInverterID] = pLimit;
}

void SetLimitOpenDTU(int pInverterID, int pLimit)
{
  if (HoyInverterSerial[pInverterID] != 0)
  {
    delay(LIMIT_DELAY_IN_SECONDS_MULTIPLE_INVERTER);
  }

  HTTPClient http;
  int relLimit = pLimit / HoyMaxPower[pInverterID] * 100;
  char url[100] = "http://";
  strcat(url, DTU_IP);
  strcat(url, "/api/limit/config");

  char data[150];
  sprintf(data, "%s%s%s%d%s", "data={\"serial\":\"", HoyInverterSerial[pInverterID], "\", \"limit_type\":1, \"limit_value\":", relLimit, "}");

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
  http.end();
  CurrentLimit[pInverterID] = pLimit;
}

void SetLimit(int pLimit)
{
  Serial.println("setting new limit to " + String(pLimit) + " Watt");
  for (int i = 0; i < INVERTER_COUNT; i++)
  {
    float Factor = HoyMaxPower[i] / GetMaxWattFromAllInverters();
    int NewLimit = int(pLimit * Factor);
    NewLimit = ApplyLimitsToSetpointInverter(i, NewLimit);
    if (USE_AHOY)
    {
      SetLimitAhoy(i, NewLimit);
    }
    else if (USE_OPENDTU)
    {
      SetLimitOpenDTU(i, NewLimit);
    }
    else
    {
      Serial.println("Error: DTU Type not defined");
    }
  }
}

void GetHoymilesStatus()
{
  HTTPClient http;
  char url[100] = "http://";
  strcat(url, DTU_IP);
  if (USE_AHOY)
  {
    strcat(url, "/api/index");
    Serial.println(url);
    http.begin(url);

    if (http.GET() == 200)
    {
      // Parsing
      DynamicJsonDocument doc(8182);
      DeserializationError error = deserializeJson(doc, http.getString());
      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      JsonArray inverters = doc["inverters"];
      // Parameters
      for (int i = 0; i < INVERTER_COUNT; i++)
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
  else if (USE_OPENDTU)
  {
    strcat(url, "/api/livedata/status");
    Serial.println(url);
    http.begin(url);

    if (http.GET() == 200)
    {
      // Parsing
      DynamicJsonDocument doc(8182);
      DeserializationError error = deserializeJson(doc, http.getString());
      if (error)
      {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }
      JsonArray inverters = doc["inverters"];
      // Parameters
      for (int i = 0; i < INVERTER_COUNT; i++)
      {
        HoyInverterSerial[i] = inverters[i]["serial"];
        HoyAvailable[i] = inverters[i]["reachable"];
        HoyLimitAbsolute[i] = inverters[i]["limit_absolute"];
        HoyACProduction[i] = inverters[i]["AC"][0]["Power"]["v"];

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

/*
void ZeroExport()
{
  int newLimitSetpoint = GetMaxWattFromAllInverters();
  float PreviousLimitSetpoint = newLimitSetpoint;

  GetHoymilesStatus();

  if (HoyAvailable)
  {
    for (int x = 0; x < (LOOP_INTERVAL_IN_SECONDS / POLL_INTERVAL_IN_SECONDS); x++)
    {
      float powermeterWatts = GetPowermeterWatts();
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
        SetLimit(newLimitSetpoint);
        if ((LOOP_INTERVAL_IN_SECONDS - LIMIT_DELAY_IN_SECONDS - x * POLL_INTERVAL_IN_SECONDS) <= 0)
        {
          break;
        }
        else
        {
          delay(LOOP_INTERVAL_IN_SECONDS - LIMIT_DELAY_IN_SECONDS - x * POLL_INTERVAL_IN_SECONDS);
        }
        break;
      }
      else
      {
        delay(POLL_INTERVAL_IN_SECONDS);
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
        float hoymilesActualPower = GetHoymilesActualPower();
        newLimitSetpoint = hoymilesActualPower + powermeterWatts - POWERMETER_TARGET_POINT;
        float LimitDifference = abs(PreviousLimitSetpoint - newLimitSetpoint);
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
        float LimitDifference = abs(PreviousLimitSetpoint - newLimitSetpoint);
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

      // producing too little power: increase limit
    }
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
      SetLimit(newLimitSetpoint);
    }
  }
  else
  {
    delay(LOOP_INTERVAL_IN_SEC);
  }
}
*/

void setup()
{
  Serial.begin(SERIAL_BAUDRATE);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Starting...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Ich verbinde mich mit dem Netzwerk...");
  }

  Serial.println("Ich bin mit dem Netzwerk verbunden!");
}

void loop()
{
  // Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > LOOP_INTERVAL_IN_SECONDS)
  {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
      GetHoymilesStatus();
      delay(1000);
      getLimitOpenDTU();
      delay(10000);
    }
    lastTime = millis();
  }
}