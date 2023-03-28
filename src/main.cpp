#include <Arduino.h>
#include "defaults.h"
#include "MessageOutput.h"
#include "ZeroExport.h"
#include "Display.h"
#include "Display_helper.h"
#include "Hoymiles.h"
#include "Network.h"
#include "PowerMeter.h"

void setup()
{
  // Initialize serial output
  Serial.begin(SERIAL_BAUDRATE);
  while (!Serial)
    yield();
  MessageOutput.println();
  MessageOutput.println(F("Starting ZeroExport"));

  // Initialize WiFi & MQTT
  MessageOutput.print(F("Initialize Network... "));
  Network.init();
  MessageOutput.println(F("done"));

  // Initialize PowerMeter
  MessageOutput.print(F("Initialize PowerMeter... "));
  PowerMeter.init("MQTT"); // MQTT / Shelly_EM3 / Tasmota
  MessageOutput.println(F("done"));

  // Initialize Hoymiles
  MessageOutput.print(F("Initialize Hoymiles... "));
  delay(5000);
  Hoymiles.init("OpenDTU");
  MessageOutput.println(F("done"));
  
  delay(1000);
  ZeroExport.init();

  Display.enablePowerSafe = false;
  Display.enableScreensaver = false;
  Display.contrast = 255;
  Display.rotation = 2;
  Display.period = 1000;
  Display.init(DisplayType_t::PCD8544, 13, 14, 15, 26, 255, 27);
}

void loop()
{
  Network.loop();
  yield();
  PowerMeter.loop();
  yield();
  ZeroExport.loop();
  yield();
  Hoymiles.loop();
  yield();
  Display.loop();
  yield();
  MessageOutput.loop();
  yield();
}