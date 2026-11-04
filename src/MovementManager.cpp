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
  if (id <= 0) {
    Serial.println(F("WS: id_comando invalido; no se modifica ninguna salida."));
    return;
  }

  // Apagado: solo apagar si el comando indicado es el que esta activo.
  if (!active) {
    if (activeMovement && activeCommandId == id) {
      Serial.printf("WS: apagando comando %d.\n", id);
      hw.allOff();
      activeMovement = nullptr;
      activeCommandId = -1;
    } else {
      Serial.printf("WS: se recibio apagado para %d, pero no es el comando activo.\n", id);
    }
    return;
  }

  // Mapeo directo de los IDs reales de la API a las salidas fisicas.
  // No depende de que GET /comandos haya llenado la cache de nombres.
  const Movement* found = nullptr;
  switch (id) {
    case 23: found = &MOVEMENTS[5]; break;  // AVANZAR - SN1 QF
    case 24: found = &MOVEMENTS[6]; break;  // RETROCEDER - SN1 QG
    case 25: found = &MOVEMENTS[8]; break;  // GIRO_DERECHA - SN2 QA
    case 26: found = &MOVEMENTS[11]; break; // GIRO_IZQUIERDA - SN2 QD
    case 27: found = &MOVEMENTS[4]; break;  // ROTAR_DERECHA - SN1 QE
    case 28: found = &MOVEMENTS[7]; break;  // ROTAR_IZQUIERDA - SN1 QH
    case 29: found = &MOVEMENTS[10]; break; // EXCAVAR - SN2 QC
    case 30: found = &MOVEMENTS[9]; break;  // SOLTAR - SN2 QB
    case 31: found = &MOVEMENTS[1]; break;  // LUZ - SN1 QB
    case 32: found = &MOVEMENTS[3]; break;  // SONIDO - SN1 QD
    case 33: found = &MOVEMENTS[0]; break;  // DEMO_DEFECTO - SN1 QA
    default:
      Serial.printf("WS: id_comando %d no tiene una salida fisica asignada.\n", id);
      return;
  }

  if (activeMovement && activeCommandId == id) {
    Serial.printf("WS: comando %d ya estaba activo; se conserva.\n", id);
    return;
  }

  // Garantizar que no queden dos movimientos activos simultaneamente.
  hw.allOff();
  hw.activate(found->reg, found->mask);
  activeMovement = found;
  activeCommandId = id;

  Serial.printf("WS: SALIDA ACTIVADA -> comando=%d, nombre=%s, registro=%s, mascara=0x%02X\n",
                id, found->apiName,
                found->reg == OutputRegister::SN1 ? "SN1" : "SN2",
                found->mask);
}

bool MovementManager::isActive() const {
  return activeMovement != nullptr;
}
