#include "MovementManager.h"

MovementManager::MovementManager(Control595& hardware, ApiClient& client)
  : hw(hardware), api(client) {}

bool MovementManager::begin() {
  return api.loadCommands();
}

const Movement* MovementManager::findByKey(char key) const {
  key = toupper((unsigned char)key);
  for (size_t i = 0; i < MOVEMENT_COUNT; ++i) {
    if (toupper((unsigned char)MOVEMENTS[i].key) == key) return &MOVEMENTS[i];
  }
  return nullptr;
}

bool MovementManager::activate(const Movement& movement) {
  int id = -1;
  if (movement.requiresApi) {
    id = api.commandId(movement.apiName);
    if (id <= 0) {
      Serial.printf("No existe un id_comando activo para '%s'. No se activa.\n",
                    movement.apiName);
      return false;
    }
  }

  hw.activate(movement.reg, movement.mask);
  activeMovement = &movement;
  activeCommandId = id;

  // Una única petición de activación. El endpoint registra el historial.
  if (movement.requiresApi && !api.updateMovementState(id, true, "ESP8266")) {
    Serial.println(F("No se pudo confirmar la activacion en la API. Se apaga por seguridad."));
    hw.allOff();
    activeMovement = nullptr;
    activeCommandId = -1;
    return false;
  }

  Serial.printf("Activado: %s (tecla %c)\n",
                movement.apiName[0] ? movement.apiName : "VELOCIDAD local",
                movement.key);
  return true;
}

void MovementManager::deactivateActive() {
  if (!activeMovement) return;

  hw.allOff();
  if (activeMovement->requiresApi && activeCommandId > 0) {
    // La desactivación NO debe generar un registro de historial según la regla acordada.
    // El endpoint actual aparentemente registra historial también al actualizar estado;
    // confirmar en backend que conectado=false no inserte una entrada.
    api.updateMovementState(activeCommandId, false, "ESP8266");
  }

  activeMovement = nullptr;
  activeCommandId = -1;
}

void MovementManager::handleKey(char key) {
  const Movement* movement = findByKey(key);
  if (!movement) return;

  if (activeMovement == movement) {
    deactivateActive();
    Serial.println(F("Movimiento apagado."));
    return;
  }

  if (activeMovement) deactivateActive();
  activate(*movement);
}

void MovementManager::stopAll() {
  deactivateActive();
  hw.allOff();
}

void MovementManager::handleRemoteState(int id, bool active) {
  if (id <= 0) return;

  if (!active) {
    // Un evento de apagado solo puede detener el comando que realmente está activo.
    if (activeMovement && activeCommandId == id) {
      hw.allOff();
      activeMovement = nullptr;
      activeCommandId = -1;
      Serial.printf("WS: comando %d desactivado.\n", id);
    }
    return;
  }

  const char* name = api.commandName(id);
  if (!name) {
    Serial.printf("WS: id_comando %d no está en la lista de comandos cargada.\n", id);
    return;
  }

  const String target(name);
  const Movement* found = nullptr;
  for (size_t i = 0; i < MOVEMENT_COUNT; ++i) {
    String candidate(MOVEMENTS[i].apiName);
    candidate.toUpperCase();
    if (candidate == target) {
      found = &MOVEMENTS[i];
      break;
    }
  }

  if (!found) {
    Serial.printf("WS: no existe salida física para '%s'.\n", name);
    return;
  }

  if (activeMovement && activeCommandId == id) return;
  hw.allOff();
  hw.activate(found->reg, found->mask);
  activeMovement = found;
  activeCommandId = id;
  Serial.printf("WS: comando remoto activado: %s (%d).\n", name, id);
}

bool MovementManager::isActive() const {
  return activeMovement != nullptr;
}
