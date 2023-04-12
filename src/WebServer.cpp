#include "WebServer.h"

WebServerClass::WebServerClass()
    : _server(80)
{
}

void WebServerClass::init()
{
    using std::placeholders::_1;

    if (!SPIFFS.begin(true))
    {
        Serial.println("An error has occurred while mounting SPIFFS");
    }
    Serial.println("SPIFFS mounted successfully");

    if (loadSettingsFromSPIFFS())
    {
        ssid = jsonDoc["wifi-ssid"].as<const char *>();
        pass = jsonDoc["wifi-password"].as<const char *>();
    }
    Serial.println(ssid);
    Serial.println(pass);

    if (initialize_Wifi(ssid, pass))
    {
        _server.on("/", HTTP_GET, std::bind(&WebServerClass::handleRoot, this, _1));
        _server.on("/", HTTP_POST, std::bind(&WebServerClass::parseSettings, this, _1));
    }
    else
    {
        Serial.println("Setting Access Point");
        WiFi.softAP("ZeroExport AP-Mode", NULL);

        _server.on("/", HTTP_GET, std::bind(&WebServerClass::handleRoot, this, _1));
        _server.on("/", HTTP_POST, std::bind(&WebServerClass::parseSettings, this, _1));
    }

    WiFi.onEvent(std::bind(&WebServerClass::WiFiEvent, this, _1));

    _server.begin();
}

void WebServerClass::loop()
{
}

/*
void WebServerClass::handleSettings()
{
    String html = readFile("settings.html");
    html.replace("{{DEVICE_NAME}}", device_name);
    html.replace("{{DEVICE_ID}}", String(device_id));
    _server.send(200, "text/html", html);
}
*/

void WebServerClass::WiFiEvent(WiFiEvent_t event)
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
        Network.connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        isConnected = false;
        break;
    default:
        break;
    }
}

bool WebServerClass::saveSettingsToSPIFFS()
{
    // Löschen der alten Konfigurationsdatei
    SPIFFS.remove("/config.json");

    // Öffnen der Konfigurationsdatei im Schreibmodus
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
        Serial.println("Failed to open config file for writing");
        return false;
    }

    // Speichern des JSON-Objekts als String in der Datei
    serializeJson(jsonDoc, configFile);

    configFile.close();
    return true;
}

bool WebServerClass::loadSettingsFromSPIFFS()
{
    // laden Sie das JSON-Objekt aus der SPIFFS-Datei
    File configFile = SPIFFS.open("/config.json", "r");

    if (!configFile)
    {
        Serial.println("Failed to open config file");
        return false;
    }

    StaticJsonDocument<1024> doc;
    auto error = deserializeJson(doc, configFile);
    configFile.close();

    if (error)
    {
        Serial.println("Failed to parse config file");
        return false;
    }
    jsonDoc = doc.as<JsonObject>();
    return true;
}

void WebServerClass::parseSettings(AsyncWebServerRequest *request)
{
    if (request->method() == HTTP_POST)
    {
        int params = request->params();
        for (int i = 0; i < params; i++)
        {
            AsyncWebParameter *p = request->getParam(i);
            if (p->isPost())
            {
                // Parameter als Schlüssel-Wert-Paar zum JSON-Objekt hinzufügen
                jsonDoc[p->name()] = p->value();
            }
        }
        if (saveSettingsToSPIFFS())
        {
            // Senden der HTTP-Antwort mit einem 200er-Statuscode
            AsyncWebServerResponse *response = request->beginResponse(200);
            response->addHeader("Content-Type", "text/plain");
            response->addHeader("Cache-Control", "no-cache");
            request->send(response);
            ESP.restart();
        }
    }
}

// Initialize WiFi
bool WebServerClass::initialize_Wifi(const char *ssid, const char *pass)
{
    if (strcmp(ssid, "") == 0)
    {
        Serial.println("Undefined SSID");
        return false;
    }

    WiFi.mode(WIFI_STA);
    // WiFi.setAutoConnect(false);
    WiFi.setAutoReconnect(true);

    WiFi.begin(ssid, pass);
    Serial.println("Connecting to WiFi...");

    unsigned long current_time = millis();
    previous_time = current_time;

    while (WiFi.status() != WL_CONNECTED)
    {
        current_time = millis();
        if (current_time - previous_time >= Delay)
        {
            Serial.println("Failed to connect.");
            return false;
        }
    }

    Serial.println(WiFi.localIP());
    return true;
}

void WebServerClass::handleRoot(AsyncWebServerRequest *request)
{
    loadFromSPIFFS(request, "/index.html.gz");
}

bool WebServerClass::loadFromSPIFFS(AsyncWebServerRequest *request, String path)
{
    String dataType = "text/html";

    if (SPIFFS.exists(path))
    {
        File dataFile = SPIFFS.open(path, "r");
        if (!dataFile)
        {
            handleNotFound(request);
            return false;
        }

        AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, dataType);

        response->addHeader("Content-Encoding", "gzip");
        request->send(response);

        dataFile.close();
    }
    else
    {
        handleNotFound(request);
        return false;
    }
    return true;
}

void WebServerClass::handleNotFound(AsyncWebServerRequest *request)
{
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";

    for (uint8_t i = 0; i < request->args(); i++)
    {
        message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }

    request->send(404, "text/plain", message);
}

WebServerClass WebServer;