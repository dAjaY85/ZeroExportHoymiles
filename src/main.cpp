#include <Arduino.h>
#include "defaults.h"

#include "ZeroExport.h"
// #include "Display.h"
// #include "Display_helper.h"
// #include "Network.h"
#include "PowerMeter.h"

WiFiClient wlanclient;
PubSubClient mqttClient(wlanclient);

int WifiFault;
bool WifiNewConnected;
int WifiReconnect = 0;

static bool StartupDone;
static bool MqttReconnect;
static bool MqttSendReconnect;
static unsigned long T_MqttReconnected;

// ------------------------
// ###### Mqtt Read ######
// ------------------------

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String str_topic = String(topic); // Char to String
  if (length > 19)
  {
    Serial.println("Message to long !!!!");
    length = 19;
  }

  Serial.println("Message arrived on Topic:");
  Serial.print(topic);
  Serial.print(" ");

  char message[20] = {0x00};

  for (int i = 0; i < length; i++)
    message[i] = (char)payload[i];

  message[length] = 0x00; // NUL Terminierung

  // Payload in Zahl wandeln
  float Value;
  Value = atof(message);
  Serial.println(Value, 2);

  // Werte Zuordnen
  if (str_topic.equals(MQTT_TOPIC))
  {
    Network.MQTT_PowerMeter = Value;
  }
}

void WiFiConnect()
{
  WiFi.hostname(APP_HOSTNAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  { // so lange hier verweilen bis neu verbunden
    WifiNewConnected = true;
    delay(250);
    Serial.print(".");
    WifiFault++;
    if (WifiFault > 30)
    {
      WifiReconnect++;
      if (WifiReconnect >= 20)
      {
        ESP.restart();
      }
      WifiFault = 0;
      Serial.println("\r\nStartWifiAgain");
      WiFi.disconnect();
      delay(500);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    delay(250);
  }
  if (WifiNewConnected)
  {
    WifiNewConnected = false;
    Serial.println("\r\nOnline");
    Serial.println("\nVerbunden mit: " + WiFi.SSID());
    Serial.println("Esp IP: " + WiFi.localIP().toString());
    Serial.println("Esp GW: " + WiFi.gatewayIP().toString());
    WiFi.persistent(true);
  }
}

void setup()
{
  Serial.begin(SERIAL_BAUDRATE);

  // Network.init();
  WiFiConnect();
  ArduinoOTA.setHostname("OTA_ZeroExport");
  //  ArduinoOTA.setPassword("OTA_ZeroExport");
  ArduinoOTA.begin();

  mqttClient.setServer(MQTT_IP, 1883); // Hier euer MQTT Broker
  mqttClient.setCallback(mqttCallback);

  if (mqttClient.connect("ZeroExport-Client", NULL, NULL))
  {

    Serial.println("Connected to MQTT Broker");
    mqttClient.publish("ZeroExport-Client", "Connected");
  }
  else
  {
    Serial.print("MQTT Broker connection failed");
    Serial.print(mqttClient.state());
    delay(200);
  }

  // auf diese Topics wird gehört
  mqttClient.subscribe(MQTT_TOPIC);

  PowerMeter.init("MQTT"); // MQTT / Shelly_EM3 / Tasmota

  ZeroExport.init("OpenDTU"); // OpenDTU / Ahoy
}

void loop()
{
  // Network.loop();

  PowerMeter.loop();

  ZeroExport.loop();

  // Display.loop();
}