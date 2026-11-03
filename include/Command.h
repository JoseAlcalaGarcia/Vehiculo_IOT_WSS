#pragma once
#include <Arduino.h>

enum class OutputRegister : uint8_t { SN1 = 1, SN2 = 2 };

struct Movement {
  const char* apiName;
  OutputRegister reg;
  uint8_t mask;
  char key;
  bool requiresApi;
};

// SN1
constexpr uint8_t OUT_DEMO_DEFECTO      = 0b00000001;
constexpr uint8_t OUT_LUZ               = 0b00000010;
constexpr uint8_t OUT_VELOCIDAD         = 0b00000100;
constexpr uint8_t OUT_SONIDO            = 0b00001000;
constexpr uint8_t OUT_ROTAR_DERECHA     = 0b00010000;
constexpr uint8_t OUT_AVANZAR           = 0b00100000;
constexpr uint8_t OUT_RETROCEDER        = 0b01000000;
constexpr uint8_t OUT_ROTAR_IZQUIERDA   = 0b10000000;

// SN2
constexpr uint8_t OUT_GIRO_DERECHA      = 0b00000001;
constexpr uint8_t OUT_SOLTAR            = 0b00000010;
constexpr uint8_t OUT_EXCAVAR           = 0b00000100;
constexpr uint8_t OUT_GIRO_IZQUIERDA    = 0b00001000;

// Las cadenas deben coincidir con los nombres devueltos por GET /comandos.
// La comparación del firmware ignora mayúsculas/minúsculas.
static const Movement MOVEMENTS[] = {
  {"DEMO_DEFECTO",    OutputRegister::SN1, OUT_DEMO_DEFECTO,    'V', true},
  {"LUZ",             OutputRegister::SN1, OUT_LUZ,             'C', true},
  // VELOCIDAD no aparece en el JSON de comandos proporcionado.
  {"",                OutputRegister::SN1, OUT_VELOCIDAD,       'X', false},
  {"SONIDO",          OutputRegister::SN1, OUT_SONIDO,          'Z', true},
  {"ROTAR_DERECHA",   OutputRegister::SN1, OUT_ROTAR_DERECHA,   'L', true},
  {"AVANZAR",         OutputRegister::SN1, OUT_AVANZAR,         'W', true},
  {"RETROCEDER",      OutputRegister::SN1, OUT_RETROCEDER,      'S', true},
  {"ROTAR_IZQUIERDA", OutputRegister::SN1, OUT_ROTAR_IZQUIERDA, 'J', true},
  {"GiRO_DERECHA",    OutputRegister::SN2, OUT_GIRO_DERECHA,    'D', true},
  {"SOLTAR",          OutputRegister::SN2, OUT_SOLTAR,          'I', true},
  {"EXCAVAR",         OutputRegister::SN2, OUT_EXCAVAR,         'K', true},
  {"GIRO_IZQUIERDA",  OutputRegister::SN2, OUT_GIRO_IZQUIERDA,  'A', true}
};

constexpr size_t MOVEMENT_COUNT = sizeof(MOVEMENTS) / sizeof(MOVEMENTS[0]);
