#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include "http-server.h"
#include "logger.h"
#include "config.h"
#include "../config.h"

ESP8266WebServer server(80);

void handleRoot();
void handleLog();
void handleStats();
void handle404();

void startHttpServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/log", HTTP_GET, handleLog);
  server.on("/stats", HTTP_GET, handleStats);
  server.onNotFound(handle404);

  server.begin();
  MDNS.addService("http", "tcp", 80);
  logger::log("HTTP server started");
}

void handleHttpClient() {
  server.handleClient();
}

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html"); // And send it to the client
  file.close();
}

void handleLog() {
  String log = "";

  for (uint8_t i = 0; i < logger::logLines.size(); i++) {
    log.concat(logger::logLines[i]);
    log.concat('\n');
  }

  server.send(200, "text/plain; charset=utf-8", log);
}

void handleStats() {
  const size_t capacity = JSON_OBJECT_SIZE(3) + sizeof(uint32_t) + sizeof(long) + sizeof(time_t);
  DynamicJsonDocument doc(capacity);
  String json;

  doc["freeHeap"] = ESP.getFreeHeap();
  doc["millis"] = millis();
  doc["time"] = time(nullptr);

  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handle404() {
  server.send(404, "text/html; charset=UTF-8", "<p>dunno sorry 🤷‍♀️</p>");
}
