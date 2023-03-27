#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <espMqttClientAsync.h>
#include <ArduinoOTA.h>
#include "defaults.h"

class NetworkClass
{
private:
    void connectToWiFi();
    void connectToMqtt();

    void WiFiEvent(WiFiEvent_t event);

    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(espMqttClientTypes::DisconnectReason reason);

    void onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len);
    void onMqttUnsubscribe(uint16_t packetId);
    void onMqttMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len, size_t index, size_t total);
    void onMqttPublish(uint16_t packetId);

    void createMqttClientObject();

    espMqttClientAsync *mqttClient = nullptr;
    bool reconnectMqtt = false;
    uint32_t lastReconnect = 0;

public:
    void init();
    void loop();

    int MQTT_PowerMeter;
};

extern NetworkClass Network;