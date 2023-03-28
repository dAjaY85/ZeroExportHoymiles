#include "Network.h"

void NetworkClass::connectToWiFi()
{
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void NetworkClass::connectToMqtt()
{
    Serial.println("Connecting to MQTT...");
    if (!mqttClient->connect())
    {
        Network.reconnectMqtt = true;
        Network.lastReconnect = millis();
        Serial.println("Connecting failed.");
    }
    else
    {
        Network.reconnectMqtt = false;
    }
}

void NetworkClass::WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        isConnected = true;
        localIP = WiFi.localIP();
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        isConnected = false;
        break;
    default:
        break;
    }
}

void NetworkClass::onMqttConnect(bool sessionPresent)
{
    Serial.println("Connected to MQTT.");
    // Serial.print("Session present: ");
    // Serial.println(sessionPresent);
    uint16_t packetIdSub = mqttClient->subscribe(POWERMETER_MQTT_TOPIC, 0);
    // Serial.print("Subscribing at QoS 2, packetId: ");
    // Serial.println(packetIdSub);

    /*
    mqttClient->publish("foo/bar", 0, true, "test 1");
    Serial.println("Publishing at QoS 0");
    uint16_t packetIdPub1 = mqttClient->publish("foo/bar", 1, true, "test 2");
    Serial.print("Publishing at QoS 1, packetId: ");
    Serial.println(packetIdPub1);
    uint16_t packetIdPub2 = mqttClient->publish("foo/bar", 2, true, "test 3");
    Serial.print("Publishing at QoS 2, packetId: ");
    Serial.println(packetIdPub2);
    */
}

void NetworkClass::onMqttDisconnect(espMqttClientTypes::DisconnectReason reason)
{
    Serial.printf("Disconnected from MQTT: %u.\n", static_cast<uint8_t>(reason));

    if (WiFi.isConnected())
    {
        Network.reconnectMqtt = true;
        Network.lastReconnect = millis();
    }
}

void NetworkClass::onMqttSubscribe(uint16_t packetId, const espMqttClientTypes::SubscribeReturncode *codes, size_t len)
{
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    for (size_t i = 0; i < len; ++i)
    {
        Serial.print("  qos: ");
        Serial.println(static_cast<uint8_t>(codes[i]));
    }
}

void NetworkClass::onMqttUnsubscribe(uint16_t packetId)
{
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void NetworkClass::onMqttMessage(const espMqttClientTypes::MessageProperties &properties, const char *topic, const uint8_t *payload, size_t len, size_t index, size_t total)
{
    (void)properties;
    (void)topic;
    (void)index;
    (void)total;

    String mqttValue = "";
    for (int i = 0; i < len; i++)
    {
        mqttValue += (char)payload[i];
    }
    MQTT_PowerMeter = mqttValue.toInt();
}

void NetworkClass::onMqttPublish(uint16_t packetId)
{
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void NetworkClass::createMqttClientObject()
{
    if (mqttClient != nullptr)
        delete mqttClient;

    mqttClient = static_cast<espMqttClientAsync *>(new espMqttClientAsync);
}

void NetworkClass::init()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    createMqttClientObject();

    WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);
    WiFi.onEvent(std::bind(&NetworkClass::WiFiEvent, this, _1));

    static_cast<espMqttClientAsync *>(mqttClient)->setServer(MQTT_IP, MQTT_PORT);
    static_cast<espMqttClientAsync *>(mqttClient)->setCredentials(MQTT_USER, MQTT_PASS);

    static_cast<espMqttClientAsync *>(mqttClient)->onConnect(std::bind(&NetworkClass::onMqttConnect, this, _1));
    static_cast<espMqttClientAsync *>(mqttClient)->onDisconnect(std::bind(&NetworkClass::onMqttDisconnect, this, _1));

    static_cast<espMqttClientAsync *>(mqttClient)->onSubscribe(std::bind(&NetworkClass::onMqttSubscribe, this, _1, _2, _3));
    static_cast<espMqttClientAsync *>(mqttClient)->onUnsubscribe(std::bind(&NetworkClass::onMqttUnsubscribe, this, _1));

    static_cast<espMqttClientAsync *>(mqttClient)->onMessage(std::bind(&NetworkClass::onMqttMessage, this, _1, _2, _3, _4, _5, _6));
    static_cast<espMqttClientAsync *>(mqttClient)->onPublish(std::bind(&NetworkClass::onMqttPublish, this, _1));

    connectToWiFi();
}

void NetworkClass::loop()
{
    if ((millis() - lastTime) > 1000)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            connectToWiFi();
        }
        lastTime = millis();
    }

    if (reconnectMqtt && millis() - lastReconnect > 5000)
    {
        connectToMqtt();
    }
}

NetworkClass Network;