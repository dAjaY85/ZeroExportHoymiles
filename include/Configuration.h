#pragma once

#include <Arduino.h>

#define CONFIG_FILENAME "/config.json"

// Number of Inverters
const uint16_t DTU_INVERTER_COUNT = 1;
const uint16_t DTU_MIN_POWER_PERCENT = 5;

class ConfigurationClass
{
private:
public:
    ConfigurationClass(/* args */);
    ~ConfigurationClass();
    void init();
    void loop();

    // MQTT
    const char *MQTT_IP = "192.168.178.xx";
    const char *MQTT_USER = "";
    const char *MQTT_PASS = "";

    // DTU SETTINGS
    const char *DTU_IP = "";
    const char *DTU_USER = "admin";
    const char *DTU_PASS = "openDTU42";

    // POWERMETER SETTINGS
    const char *POWERMETER_MQTT_TOPIC = "vzlogger/data/chn2/raw";
    const char *POWERMETER_IP = "192.168.178.xxx";
};

extern ConfigurationClass Configuration;