#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <AsyncJson.h>
#include "Network.h"
#include "Configuration.h"

class WebServerClass
{
public:
    bool isConnected = false;
    IPAddress localIP;
    WebServerClass();

    void init();
    void loop();

private:
    AsyncWebServer _server;
    StaticJsonDocument<1024> jsonDoc;

    void handleRoot(AsyncWebServerRequest *request);
    void handleNotFound(AsyncWebServerRequest *request);

    void parseSettings(AsyncWebServerRequest *request);
    void WiFiEvent(WiFiEvent_t event);

    bool initialize_Wifi(const char *ssid, const char *pass);
    bool loadFromSPIFFS(AsyncWebServerRequest *request, String path);

    bool loadSettingsFromSPIFFS();
    bool saveSettingsToSPIFFS();

    unsigned long previous_time = 0;
    const long Delay = 10000;

    const char *ssid = "";
    const char *pass = "";
};

extern WebServerClass WebServer;