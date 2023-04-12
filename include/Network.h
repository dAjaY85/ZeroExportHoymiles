#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <espMqttClientAsync.h>
#include <ArduinoOTA.h>
#include "defaults.h"
#include "Configuration.h"

class NetworkClass
{
private:
    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);

    void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len);
    void onMqttUnsubscribe(uint16_t packetId);
    void onMqttMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len, size_t index, size_t total);
    void onMqttPublish(uint16_t packetId);

    void createMqttClientObject();

    espMqttClientAsync *mqttClient = nullptr;
    uint32_t lastReconnect = 0;
    uint32_t lastTime = 0;

public:
    void init();
    void loop();

    bool reconnectMqtt = false;
    int MQTT_PowerMeter;

    void connectToMqtt();
};

extern NetworkClass Network;