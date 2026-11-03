#include "DemoManager.h"

DemoManager::DemoManager(Control595& hardware, ApiClient& client,
                         MovementManager& movementManager)
  : hw(hardware), api(client), movements(movementManager) {}

bool DemoManager::loadSequence(char key, const char*& apiDemoName) {
  key = toupper((unsigned char)key);
  if (key == 'Q') {
    // Nombre real devuelto por GET /vehiculos/7/demos.
    apiDemoName = "DEMO_8_MOV";
    return true;
  }
  if (key == 'E') {
    apiDemoName = "DEMO_POSICION";
    return true;
  }
  if (key == 'R') {
    apiDemoName = "DEMO_ACCION";
    return true;
  }
  return false;
}

bool DemoManager::start(char key) {
  if (active) return false;

  const char* apiName = nullptr;
  if (!loadSequence(key, apiName)) return false;

  int demoId = -1;
  if (!api.getDemoId(apiName, demoId)) {
    Serial.println(F("Demo no iniciado: no se obtuvo su id_demo desde la API."));
    return false;
  }

  // Marcarla activa antes del POST evita una carrera si el primer evento
  // WebSocket llega inmediatamente después de que la API acepte la solicitud.
  active = true;
  activeDemoId = demoId;
  activeCommandId = -1;

  // La API transmite por WebSocket cada movimiento, duración y pausa.
  // No se ejecuta una secuencia paralela local para evitar movimientos duplicados.
  if (!api.executeDemo(demoId)) {
    active = false;
    activeDemoId = -1;
    Serial.println(F("La API rechazo la ejecucion del demo."));
    return false;
  }
  Serial.printf("Solicitud de demo %s enviada (id_demo=%d). Esperando WebSocket.\n",
                apiName, demoId);
  return true;
}

void DemoManager::handleRemoteMovement(int demoId, int commandId,
                                       uint32_t durationMs) {
  if (demoId <= 0 || commandId <= 0) return;

  // Un mensaje nuevo puede reemplazar el movimiento anterior de la misma demo.
  if (activeCommandId > 0 && activeCommandId != commandId) {
    movements.handleRemoteState(activeCommandId, false);
  }

  active = true;
  activeDemoId = demoId;
  activeCommandId = commandId;
  commandStartedAt = millis();
  commandDurationMs = durationMs;
  movements.handleRemoteState(commandId, true);

  Serial.printf("WS DEMO %d: comando %d durante %lu ms.\n",
                demoId, commandId, (unsigned long)durationMs);
}

void DemoManager::loop() {
  if (!active || activeCommandId <= 0) return;
  if ((uint32_t)(millis() - commandStartedAt) >= commandDurationMs) {
    movements.handleRemoteState(activeCommandId, false);
    activeCommandId = -1;
  }
}

void DemoManager::finishRemote() {
  if (activeCommandId > 0) {
    movements.handleRemoteState(activeCommandId, false);
  }
  activeCommandId = -1;
  activeDemoId = -1;
  active = false;
  hw.allOff();
  Serial.println(F("WS: demo finalizada; todas las salidas apagadas."));
}

void DemoManager::cancel() {
  finishRemote();
  Serial.println(F("Demo cancelada localmente."));
}

bool DemoManager::running() const {
  return active;
}
