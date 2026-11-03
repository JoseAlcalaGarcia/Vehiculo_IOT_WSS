#include "Control595.h"
#include "Config.h"

void Control595::begin() {
  pinMode(Config::PIN_DATA, OUTPUT);
  pinMode(Config::PIN_CLOCK, OUTPUT);
  pinMode(Config::PIN_LATCH, OUTPUT);
  digitalWrite(Config::PIN_LATCH, HIGH);
  allOff();
}

void Control595::writeRegisters(uint8_t sn2, uint8_t sn1) {
  digitalWrite(Config::PIN_LATCH, LOW);
  shiftOut(Config::PIN_DATA, Config::PIN_CLOCK, MSBFIRST, sn2);
  shiftOut(Config::PIN_DATA, Config::PIN_CLOCK, MSBFIRST, sn1);
  digitalWrite(Config::PIN_LATCH, HIGH);
}

void Control595::activate(OutputRegister reg, uint8_t mask) {
  if (reg == OutputRegister::SN1) writeRegisters(0x00, mask);
  else writeRegisters(mask, 0x00);
}

void Control595::allOff() {
  writeRegisters(0x00, 0x00);
}
