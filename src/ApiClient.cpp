#include "ApiClient.h"
#include "Config.h"
#include <ESP8266WiFi.h>

String ApiClient::normalize(const String& value) const {
  String out = value;
  out.trim();
  out.toUpperCase();
  out.replace(" ", "_");
  return out;
}

bool ApiClient::request(const String& method, const String& path,
                        const String& body, String& response, int& statusCode) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("API: no hay conexion Wi-Fi."));
    return false;
  }

  // IMPORTANTE: setInsecure() permite probar HTTPS sin validar el certificado.
  // Antes de desplegar el vehículo, sustituirlo por setTrustAnchors() con la
  // CA raíz adecuada para el certificado de iot-alcala.duckdns.org.
  BearSSL::WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(Config::HTTP_TIMEOUT_MS);
  const String url = String(Config::API_BASE) + path;

  if (!http.begin(client, url)) {
    Serial.println(F("API: no se pudo iniciar la conexion HTTPS."));
    return false;
  }

  http.addHeader(F("Content-Type"), F("application/json"));
  if (method == "GET") statusCode = http.GET();
  else if (method == "POST") statusCode = http.POST(body);
  else {
    http.end();
    return false;
  }

  if (statusCode > 0) response = http.getString();
  else Serial.printf("API: error HTTPClient %d\n", statusCode);

  http.end();
  return statusCode >= 200 && statusCode < 300;
}

bool ApiClient::loadCommands() {
  String response;
  int status = 0;
  commandCount = 0;

  if (!request("GET", "/vehiculos/7/comandos", "", response, status)) {
    Serial.printf("No se pudieron cargar comandos. HTTP=%d\n", status);
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error || !doc.is<JsonArray>()) {
    Serial.println(F("JSON de comandos invalido: se esperaba un arreglo."));
    Serial.println(response);
    return false;
  }

  for (JsonObject item : doc.as<JsonArray>()) {
    if (commandCount >= MAX_COMMANDS) break;
    const char* name = item["nombre"] | "";
    int id = item["id_comando"] | -1;
    int vehicle = item["id_vehiculo"] | -1;
    int active = item["activo"] | 0;

    if (name[0] && id > 0 && vehicle == Config::VEHICLE_ID && active == 1) {
      commands[commandCount].name = normalize(String(name));
      commands[commandCount].id = id;
      ++commandCount;
    }
  }

  Serial.printf("Comandos activos cargados: %u\n", (unsigned)commandCount);
  return commandCount > 0;
}

const char* ApiClient::commandName(int id) const {
  for (size_t i = 0; i < commandCount; ++i) {
    if (commands[i].id == id) return commands[i].name.c_str();
  }
  return nullptr;
}

int ApiClient::commandId(const char* apiName) const {
  const String target = normalize(String(apiName));
  for (size_t i = 0; i < commandCount; ++i) {
    if (commands[i].name == target) return commands[i].id;
  }
  return -1;
}

bool ApiClient::updateMovementState(int id, bool active, const char* origin) {
  if (id <= 0) return false;

  JsonDocument doc;
  doc["id_comando"] = id;
  doc["conectado"] = active;
  doc["origen"] = origin;

  String body;
  serializeJson(doc, body);

  String response;
  int status = 0;
  bool ok = request("POST", "/vehiculos/7/estado", body, response, status);
  Serial.printf("Estado comando %d => %s (HTTP %d)\n",
                id, active ? "ACTIVO" : "INACTIVO", status);
  if (!response.isEmpty()) Serial.println(response);
  return ok;
}

static bool findDemoRecursive(JsonVariantConst value, const String& wanted, int& foundId) {
  if (value.is<JsonArrayConst>()) {
    for (JsonVariantConst child : value.as<JsonArrayConst>()) {
      if (findDemoRecursive(child, wanted, foundId)) return true;
    }
  } else if (value.is<JsonObjectConst>()) {
    JsonObjectConst obj = value.as<JsonObjectConst>();
    int id = obj["id_demo"] | -1;
    const char* name1 = obj["nombre"] | "";
    const char* name2 = obj["nombre_demo"] | "";
    String name = name1[0] ? String(name1) : String(name2);

    if (id > 0 && name.length()) {
      name.trim();
      name.toUpperCase();
      name.replace(" ", "_");
      String target = wanted;
      target.toUpperCase();
      target.replace(" ", "_");
      if (name == target) {
        foundId = id;
        return true;
      }
    }
    for (JsonPairConst pair : obj) {
      if (findDemoRecursive(pair.value(), wanted, foundId)) return true;
    }
  }
  return false;
}

bool ApiClient::getDemoId(const char* requestedName, int& demoId) {
  String response;
  int status = 0;
  if (!request("GET", "/vehiculos/7/demos", "", response, status)) {
    Serial.printf("No se pudieron consultar demos. HTTP=%d\n", status);
    return false;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.println(F("JSON de demos no reconocido:"));
    Serial.println(response);
    return false;
  }

  if (!findDemoRecursive(doc.as<JsonVariantConst>(), String(requestedName), demoId)) {
    Serial.printf("No se encontro el demo '%s'. Respuesta recibida:\n", requestedName);
    Serial.println(response);
    return false;
  }
  return true;
}

bool ApiClient::executeDemo(int demoId) {
  if (demoId <= 0) return false;
  String response;
  int status = 0;
  String path = "/vehiculos/7/demos/" + String(demoId) + "/ejecutar";
  bool ok = request("POST", path, "{}", response, status);
  Serial.printf("Ejecutar demo %d (HTTP %d)\n", demoId, status);
  if (!response.isEmpty()) Serial.println(response);
  return ok;
}

bool ApiClient::reportVehicleInfo() {
  String response;
  int status = 0;
  bool ok = request("GET", "/vehiculos/7", "", response, status);
  if (ok) {
    Serial.println(F("API de vehiculo disponible:"));
    Serial.println(response);
  }
  return ok;
}
