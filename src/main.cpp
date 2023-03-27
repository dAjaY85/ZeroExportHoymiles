#include <Arduino.h>
#include "defaults.h"

#include "ZeroExport.h"
// #include "Display.h"
// #include "Display_helper.h"
#include "Network.h"
#include "PowerMeter.h"

void setup()
{
  Serial.begin(SERIAL_BAUDRATE);

  Network.init();

  PowerMeter.init("MQTT"); // MQTT / Shelly_EM3 / Tasmota

  ZeroExport.init("OpenDTU"); // OpenDTU / Ahoy
}

void loop()
{
  Network.loop();

  PowerMeter.loop();

  ZeroExport.loop();

  // Display.loop();
}