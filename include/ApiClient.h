#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

class ApiClient {
public:
  bool loadCommands();
  int commandId(const char* apiName) const;
  const char* commandName(int id) const;
  bool updateMovementState(int commandId, bool active, const char* origin = "MANUAL");
  bool getDemoId(const char* requestedName, int& demoId);
  bool executeDemo(int demoId);
  bool reportVehicleInfo();
private:
  struct CommandEntry {
    String name;
    int id = -1;
  };
  static constexpr size_t MAX_COMMANDS = 24;
  CommandEntry commands[MAX_COMMANDS];
  size_t commandCount = 0;

  bool request(const String& method, const String& path,
               const String& body, String& response, int& statusCode);
  String normalize(const String& value) const;
};
