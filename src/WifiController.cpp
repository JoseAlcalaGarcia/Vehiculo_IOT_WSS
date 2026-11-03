#include "WifiController.h"
#include "Config.h"

void WifiController::begin() {
  pinMode(Config::PIN_WIFI_RESET, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  String suffix = WiFi.macAddress();
  suffix.replace(":", "");
  suffix = suffix.substring(suffix.length() > 6 ? suffix.length() - 6 : 0);
  String apName = "EXCAVADORA-" + suffix;

  manager.setConfigPortalTimeout(180);
  manager.setConnectTimeout(20);

  Serial.println(F("\nIniciando Wi-Fi..."));
  if (!manager.autoConnect(apName.c_str(), Config::AP_PASSWORD)) {
    Serial.println(F("No se establecio Wi-Fi. Se reiniciara para reintentar."));
    delay(1000);
    ESP.restart();
  }

  Serial.println(F("Wi-Fi conectado."));
  Serial.print(F("SSID: ")); Serial.println(WiFi.SSID());
  Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
  Serial.print(F("MAC: ")); Serial.println(WiFi.macAddress());
}

void WifiController::loop() {
  const uint32_t now = millis();

  if (digitalRead(Config::PIN_WIFI_RESET) == LOW) {
    if (resetPressedAt == 0) resetPressedAt = now;
    if (!resetHandled && now - resetPressedAt >= Config::WIFI_RESET_HOLD_MS) {
      resetHandled = true;
      resetCredentials();
    }
  } else {
    resetPressedAt = 0;
    resetHandled = false;
  }

  if (WiFi.status() != WL_CONNECTED &&
      now - lastRetry >= Config::WIFI_RETRY_INTERVAL_MS) {
    lastRetry = now;
    Serial.println(F("Wi-Fi desconectado. Reintentando..."));
    WiFi.disconnect();
    WiFi.begin();
  }
}

bool WifiController::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

void WifiController::resetCredentials() {
  Serial.println(F("Restableciendo credenciales Wi-Fi..."));
  manager.resetSettings();
  WiFi.disconnect(true);
  delay(500);
  ESP.restart();
}
