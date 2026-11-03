#include <Arduino.h>
#include "Config.h"
#include "WifiController.h"
#include "Control595.h"
#include "ApiClient.h"
#include "MovementManager.h"
#include "DemoManager.h"
#include "WebSocketController.h"

WifiController wifi;
Control595 control;
ApiClient api;
MovementManager movements(control, api);
DemoManager demos(control, api, movements);
WebSocketController websocket(movements, demos);

bool wifiWasConnected = false;
bool apiWasAvailable = false;
bool commandsLoaded = false;
bool websocketStarted = false;
uint32_t lastApiRetry = 0;

void printHelp() {
  Serial.println();
  Serial.println(F("=== EXCAVADORA IoT - NODEMCU ESP8266 ==="));
  Serial.println(F("SN1: V=DEMO_DEFECTO C=LUZ X=VELOCIDAD Z=SONIDO"));
  Serial.println(F("     L=ROTAR_DERECHA W=AVANZAR S=RETROCEDER J=ROTAR_IZQUIERDA"));
  Serial.println(F("SN2: D=GIRO_DERECHA I=SOLTAR K=EXCAVAR A=GIRO_IZQUIERDA"));
  Serial.println(F("DEMOS: Q=8 movimientos E=posicion R=accion"));
  Serial.println(F("Wi-Fi: mantener boton D6 5 segundos o enviar ! para borrar credenciales"));
  Serial.println(F("Control remoto: eventos de la API mediante WSS."));
  Serial.println(F("========================================"));
}

void setup() {
  Serial.begin(115200);
  delay(200);

  control.begin();
  randomSeed(ESP.getChipId() ^ micros());

  wifi.begin();
  printHelp();

  if (wifi.connected()) {
    apiWasAvailable = api.reportVehicleInfo();
    if (apiWasAvailable) commandsLoaded = movements.begin();
    websocket.begin();
    websocketStarted = true;
  }
  wifiWasConnected = wifi.connected();
}

void loop() {
  wifi.loop();

  const bool wifiNowConnected = (WiFi.status() == WL_CONNECTED);
  if (wifiWasConnected && !wifiNowConnected) {
    movements.stopAll();
    demos.cancel();
    apiWasAvailable = false;
    commandsLoaded = false;
    control.allOff();
    Serial.println(F("Wi-Fi perdido: salidas detenidas."));
  }

  if (!wifiWasConnected && wifiNowConnected) {
    Serial.println(F("Wi-Fi restablecido. Cargando comandos..."));
    apiWasAvailable = api.reportVehicleInfo();
    if (apiWasAvailable) commandsLoaded = movements.begin();
  }
  wifiWasConnected = wifiNowConnected;

  if (wifiNowConnected) {
    if (!websocketStarted) {
      websocket.begin();
      websocketStarted = true;
    }
    websocket.loop();

    if ((!apiWasAvailable || !commandsLoaded) &&
        millis() - lastApiRetry >= 30000) {
      lastApiRetry = millis();
      apiWasAvailable = api.reportVehicleInfo();
      if (apiWasAvailable) commandsLoaded = movements.begin();
    }
  }

  demos.loop();

  while (Serial.available() > 0) {
    char key = Serial.read();
    if (key == '\r' || key == '\n') continue;

    if (key == '!') {
      wifi.resetCredentials();
      return;
    }

    if (demos.running()) {
      Serial.println(F("Demo en ejecucion: tecla ignorada."));
      continue;
    }

    if (key == 'Q' || key == 'q' || key == 'E' || key == 'e' ||
        key == 'R' || key == 'r') {
      movements.stopAll();
      demos.start(key);
      continue;
    }

    movements.handleKey(key);
  }

  yield();
}
