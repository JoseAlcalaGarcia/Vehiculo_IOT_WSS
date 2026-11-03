#pragma once
#include <Arduino.h>
#include "ApiClient.h"
#include "Control595.h"
#include "MovementManager.h"
#include "Command.h"

class DemoManager {
public:
  DemoManager(Control595& hardware, ApiClient& client, MovementManager& movementManager);
  bool start(char key);
  void loop();
  void cancel();
  void handleRemoteMovement(int demoId, int commandId, uint32_t durationMs);
  void finishRemote();
  bool running() const;
private:
  Control595& hw;
  ApiClient& api;
  MovementManager& movements;
  bool active = false;
  int activeDemoId = -1;
  int activeCommandId = -1;
  uint32_t commandStartedAt = 0;
  uint32_t commandDurationMs = 0;

  bool loadSequence(char key, const char*& apiDemoName);
};
