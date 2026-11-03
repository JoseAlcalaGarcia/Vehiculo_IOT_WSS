#pragma once
#include <Arduino.h>

namespace Config {
  constexpr uint8_t VEHICLE_ID = 7;
  constexpr char API_BASE[] = "https://iot-alcala.duckdns.org/api";

  // Wi-Fi portal
  constexpr char AP_PASSWORD[] = "Excavadora2026";
  constexpr uint32_t WIFI_RETRY_INTERVAL_MS = 60000;
  constexpr uint32_t WIFI_RESET_HOLD_MS = 5000;

  // SN74HC595 (NodeMCU ESP8266)
  constexpr uint8_t PIN_CLOCK = D0;
  constexpr uint8_t PIN_LATCH = D1;
  constexpr uint8_t PIN_DATA  = D2;

  // Botón a GND. D6/GPIO12 no se utiliza en los registros.
  constexpr uint8_t PIN_WIFI_RESET = D6;

  constexpr uint32_t HTTP_TIMEOUT_MS = 3000;
  constexpr char WS_HOST[] = "iot-alcala.duckdns.org";
  constexpr uint16_t WS_PORT = 443;
  constexpr char WS_PATH[] = "/ws";
  constexpr uint32_t WS_RECONNECT_MS = 5000;
}
