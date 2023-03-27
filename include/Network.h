#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "defaults.h"

class NetworkClass
{
private:
    void WiFiConnect();
    static void mqttCallback(char *topic, byte *payload, unsigned int length);

    WiFiClient wlanclient;
    PubSubClient *mqttClient;

    int WifiFault;
    bool WifiNewConnected;
    int WifiReconnect = 0;

    bool StartupDone;
    bool MqttReconnect;
    bool MqttSendReconnect;
    uint16_t T_MqttReconnected;

public:
    void init();
    void loop();

    int MQTT_PowerMeter;
};

extern NetworkClass Network;