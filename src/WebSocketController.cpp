#include "WebSocketController.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

WebSocketController* WebSocketController::instance = nullptr;

WebSocketController::WebSocketController(MovementManager& movementManager,
                                         DemoManager& demoManager)
  : movements(movementManager), demos(demoManager) {}

void WebSocketController::begin() {
  instance = this;
  client.onEvent(WebSocketController::eventCallback);
  client.setReconnectInterval(Config::WS_RECONNECT_MS);
  client.enableHeartbeat(15000, 3000, 2);

  // beginSSL sin fingerprint utiliza TLS sin validación del certificado.
  // Para producción debe configurarse la CA del certificado del servidor.
  client.beginSSL(Config::WS_HOST, Config::WS_PORT, Config::WS_PATH);
  Serial.printf("WSS: conectando a %s:%u%s\n",
                Config::WS_HOST, Config::WS_PORT, Config::WS_PATH);
}

void WebSocketController::loop() {
  if (WiFi.status() == WL_CONNECTED) client.loop();
}

void WebSocketController::eventCallback(WStype_t type, uint8_t* payload,
                                        size_t length) {
  if (instance) instance->handleEvent(type, payload, length);
}

void WebSocketController::handleEvent(WStype_t type, uint8_t* payload,
                                      size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println(F("WSS: conectado a la API."));
      break;
    case WStype_DISCONNECTED:
      Serial.println(F("WSS: desconectado; se reintentara automaticamente."));
      break;
    case WStype_TEXT:
      handleText(payload, length);
      break;
    case WStype_ERROR:
      Serial.println(F("WSS: error de conexion."));
      break;
    default:
      break;
  }
}

void WebSocketController::handleText(uint8_t* payload, size_t length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.printf("WSS: JSON invalido: %s\n", error.c_str());
    return;
  }

  const char* type = doc["tipo"] | "";
  const char* action = doc["accion"] | "";
  JsonObjectConst data = doc["datos"].as<JsonObjectConst>();

  if (strcmp(type, "estado_vehiculo") == 0) {
    const int vehicleId = data["id_vehiculo"] | -1;
    const int commandId = data["id_comando"] | -1;
    const bool connected = data["conectado"] | false;
    const char* origin = data["origen"] | "";

    if (vehicleId != Config::VEHICLE_ID || commandId <= 0) return;

    // El ESP8266 informa sus propios cambios por REST. No debe volver a
    // ejecutar el evento que él mismo originó.
    if (strcmp(origin, "ESP8266") == 0) return;

    // Un movimiento remoto manual interrumpe cualquier demo en curso.
    if (connected && demos.running()) demos.cancel();
    movements.handleRemoteState(commandId, connected);
    return;
  }

  if (strcmp(type, "demo") == 0) {
    if (strcmp(action, "ejecutar_movimiento") == 0) {
      const int vehicleId = data["id_vehiculo"] | -1;
      const int demoId = data["id_demo"] | -1;
      const int commandId = data["id_comando"] | -1;
      const uint32_t duration = data["duracion_ms"] | 0;

      if (vehicleId != Config::VEHICLE_ID) return;
      demos.handleRemoteMovement(demoId, commandId, duration);
      return;
    }

    if (strcmp(action, "finalizada") == 0) {
      demos.finishRemote();
      return;
    }

    if (strcmp(action, "cancelada") == 0) {
      demos.finishRemote();
      return;
    }
  }
}
