#pragma once

#define SERIAL_BAUDRATE 115200
#define APP_HOSTNAME "ZeroExport-%06X"
#define HTTP_PORT 80

// WIFI
#define WIFI_SSID ""
#define WIFI_PASSWORD ""

// MQTT
#define MQTT_IP IPAddress(192, 168, 178, 20)
#define MQTT_PORT 1883
#define MQTT_USER "MQTT"
#define MQTT_PASS ""
#define MQTT_TOPIC "vzlogger/data/chn2/raw"

// DTU SETTINGS
#define DTU_IP "192.168.178.100"
#define DTU_USER "admin"
#define DTU_PASS "openDTU42"
// Number of Inverters
#define DTU_INVERTER_COUNT 2
#define DTU_MIN_POWER_PERCENT 5

// POWERMETER SETTINGS
#define POWERMETER_IP
#define POWERMETER_JSON_STATUS
#define POWERMETER_JSON_PAYLOAD_MQTT_PREFIX
#define POWERMETER_JSON_POWER_MQTT_LABEL