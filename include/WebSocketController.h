#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>
#include "MovementManager.h"
#include "DemoManager.h"

class WebSocketController {
public:
  WebSocketController(MovementManager& movementManager, DemoManager& demoManager);
  void begin();
  void loop();

private:
  WebSocketsClient client;
  MovementManager& movements;
  DemoManager& demos;
  static WebSocketController* instance;
  static void eventCallback(WStype_t type, uint8_t* payload, size_t length);
  void handleEvent(WStype_t type, uint8_t* payload, size_t length);
  void handleText(uint8_t* payload, size_t length);
};
