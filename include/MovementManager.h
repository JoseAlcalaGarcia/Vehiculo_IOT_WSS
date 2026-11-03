#pragma once
#include <Arduino.h>
#include "ApiClient.h"
#include "Control595.h"
#include "Command.h"

class MovementManager {
public:
  MovementManager(Control595& hardware, ApiClient& api);
  bool begin();
  void handleKey(char key);
  void stopAll();
  void handleRemoteState(int commandId, bool active);
  bool isActive() const;
private:
  Control595& hw;
  ApiClient& api;
  const Movement* activeMovement = nullptr;
  int activeCommandId = -1;
  bool activate(const Movement& movement);
  void deactivateActive();
  const Movement* findByKey(char key) const;
};
