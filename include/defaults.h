#pragma once

#define SERIAL_BAUDRATE 115200
#define APP_HOSTNAME "ZeroExport-%06X"
#define HTTP_PORT 80

// WIFI
#define WIFI_SSID "Fritz!Box"
#define WIFI_PASSWORD ""

// MQTT
#define MQTT_IP IPAddress(192, 168, 178, xx)
#define MQTT_PORT 1883
#define MQTT_USER "MQTT"
#define MQTT_PASS ""

// DTU SETTINGS
#define DTU_IP "192.168.178.100"
#define DTU_USER "admin"
#define DTU_PASS "openDTU42"
// Number of Inverters
#define DTU_INVERTER_COUNT 1
#define DTU_MIN_POWER_PERCENT 5

// POWERMETER SETTINGS
#define POWERMETER_MQTT_TOPIC "vzlogger/data/chn2/raw"
#define POWERMETER_IP "192.168.178.xxx"