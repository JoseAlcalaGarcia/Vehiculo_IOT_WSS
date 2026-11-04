#include "WebSocketController.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <string.h>

WebSocketController* WebSocketController::instance = nullptr;

WebSocketController::WebSocketController(MovementManager& movementManager,
                                         DemoManager& demoManager)
  : movements(movementManager), demos(demoManager) {}

void WebSocketController::begin() {
  instance = this;
  client.onEvent(WebSocketController::eventCallback);
  client.setReconnectInterval(Config::WS_RECONNECT_MS);
  client.enableHeartbeat(15000, 3000, 2);

  // beginSSL sin fingerprint utiliza TLS sin validacion del certificado.
  // Para produccion debe configurarse la CA del certificado del servidor.
  client.beginSSL(Config::WS_HOST, Config::WS_PORT, Config::WS_PATH);

  Serial.printf("[WSS] Iniciando conexion a wss://%s:%u%s\n",
                Config::WS_HOST, Config::WS_PORT, Config::WS_PATH);
}

void WebSocketController::loop() {
  if (WiFi.status() == WL_CONNECTED) {
    client.loop();
  }
}

void WebSocketController::eventCallback(WStype_t type, uint8_t* payload,
                                        size_t length) {
  if (instance) {
    instance->handleEvent(type, payload, length);
  }
}

void WebSocketController::handleEvent(WStype_t type, uint8_t* payload,
                                      size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println();
      Serial.println(F("[WSS] ========================================"));
      Serial.println(F("[WSS] CONEXION ESTABLECIDA CON LA API"));
      Serial.println(F("[WSS] Esperando notificaciones de los endpoints..."));
      Serial.println(F("[WSS] ========================================"));
      break;

    case WStype_DISCONNECTED:
      Serial.println();
      Serial.println(F("[WSS] *** DESCONECTADO DE LA API ***"));
      Serial.println(F("[WSS] Se intentara reconectar automaticamente."));
      break;

    case WStype_TEXT:
      // Imprime el mensaje original completo ANTES de interpretarlo.
      Serial.println();
      Serial.println(F("[WSS] >>> NOTIFICACION RECIBIDA <<<"));
      Serial.printf("[WSS] Bytes recibidos: %u\n", (unsigned int)length);
      Serial.println(F("[WSS] Contenido recibido:"));
      Serial.write(payload, length);
      Serial.println();
      Serial.println(F("[WSS] ----------------------------------------"));

      handleText(payload, length);

      Serial.println(F("[WSS] >>> FIN DE PROCESAMIENTO <<<"));
      Serial.println();
      break;

    case WStype_BIN:
      Serial.printf("[WSS] Se recibieron %u bytes binarios.\n",
                    (unsigned int)length);
      break;

    case WStype_ERROR:
      Serial.println(F("[WSS] ERROR en la conexion WebSocket/TLS."));
      if (payload && length > 0) {
        Serial.print(F("[WSS] Detalle: "));
        Serial.write(payload, length);
        Serial.println();
      }
      break;

    default:
      break;
  }
}

void WebSocketController::handleText(uint8_t* payload, size_t length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.printf("[WSS] ERROR: JSON invalido: %s\n", error.c_str());
    return;
  }

  const char* type = doc["tipo"] | "";
  const char* action = doc["accion"] | "";
  JsonObjectConst data = doc["datos"].as<JsonObjectConst>();

  Serial.printf("[WSS] Evento interpretado: tipo='%s', accion='%s'\n",
                type, action);

  if (strcmp(type, "estado_vehiculo") == 0) {
    const int vehicleId = data["id_vehiculo"] | -1;
    const int commandId = data["id_comando"] | -1;
    const bool connected = data["conectado"] | false;
    const char* origin = data["origen"] | "";

    Serial.printf(
      "[WSS] Estado: vehiculo=%d, comando=%d, conectado=%s, origen='%s'\n",
      vehicleId, commandId, connected ? "true" : "false", origin
    );

    if (vehicleId != Config::VEHICLE_ID) {
      Serial.printf("[WSS] Ignorado: el mensaje es para el vehiculo %d; este equipo es %d.\n",
                    vehicleId, Config::VEHICLE_ID);
      return;
    }

    if (commandId <= 0) {
      Serial.println(F("[WSS] Ignorado: id_comando no valido."));
      return;
    }

    // El ESP8266 informa sus propios cambios por REST. No debe ejecutar
    // nuevamente los eventos que él mismo originó.
    if (strcmp(origin, "ESP8266") == 0) {
      Serial.println(F("[WSS] Evento propio: recibido e impreso, no se ejecuta otra vez."));
      return;
    }

    if (connected && demos.running()) {
      Serial.println(F("[WSS] Se cancela la DEMO activa por un comando remoto."));
      demos.cancel();
    }

    movements.handleRemoteState(commandId, connected);
    return;
  }

  if (strcmp(type, "demo") == 0) {
    if (strcmp(action, "ejecutar_movimiento") == 0) {
      const int vehicleId = data["id_vehiculo"] | -1;
      const int demoId = data["id_demo"] | -1;
      const int commandId = data["id_comando"] | -1;
      const uint32_t duration = data["duracion_ms"] | 0;
      const uint32_t pause = data["pausa_ms"] | 0;

      Serial.printf(
        "[WSS] DEMO: vehiculo=%d, demo=%d, comando=%d, duracion=%lu ms, pausa=%lu ms\n",
        vehicleId, demoId, commandId,
        (unsigned long)duration, (unsigned long)pause
      );

      if (vehicleId != Config::VEHICLE_ID) {
        Serial.printf("[WSS] Movimiento de DEMO ignorado: este equipo corresponde al vehiculo %d.\n",
                      Config::VEHICLE_ID);
        return;
      }

      demos.handleRemoteMovement(demoId, commandId, duration);
      return;
    }

    if (strcmp(action, "finalizada") == 0) {
      Serial.println(F("[WSS] La API notifico que la DEMO finalizo."));
      demos.finishRemote();
      return;
    }

    if (strcmp(action, "cancelada") == 0) {
      Serial.println(F("[WSS] La API notifico que la DEMO fue cancelada."));
      demos.finishRemote();
      return;
    }
  }

  Serial.printf("[WSS] Notificacion recibida pero sin manejador: tipo='%s', accion='%s'.\n",
                type, action);
}
