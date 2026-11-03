#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

class WifiController {
public:
  void begin();
  void loop();
  bool connected() const;
  void resetCredentials();
private:
  WiFiManager manager;
  uint32_t lastRetry = 0;
  uint32_t resetPressedAt = 0;
  bool resetHandled = false;
};
