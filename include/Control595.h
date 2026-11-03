#pragma once
#include <Arduino.h>
#include "Command.h"

class Control595 {
public:
  void begin();
  void activate(OutputRegister reg, uint8_t mask);
  void allOff();
private:
  void writeRegisters(uint8_t sn2, uint8_t sn1);
};
