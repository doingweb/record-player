#include <ArduinoJson.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include "http-server.h"
#include "logger.h"
#include "spotify.h"
#include "../config.h"

ESP8266WebServer server(80);

void handleRoot();
void handleAuth();
void handleAuthCallback();
void handleConfig();
void handleLog();
void handleStats();
void handle404();
void redirectTo(String);

void startHttpServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/auth", HTTP_GET, handleAuth);
  server.on("/auth-callback", HTTP_GET, handleAuthCallback);
  server.on("/config", HTTP_GET, handleConfig);
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
  File file = SPIFFS.open("/index.html", "r");
  server.streamFile(file, "text/html"); // And send it to the client
  file.close();
}

void handleAuth() {
  redirectTo(getAuthorizeUrl());
}

void handleAuthCallback() {
  String code = server.arg("code");
  String state = server.arg("state");

  receiveAuthCode(code, state);

  redirectTo("/");
}

void handleConfig() {
  // Count of `doc` assignments below, plus space for copying each thing
  const size_t capacity = JSON_OBJECT_SIZE(7) +
    sizeof(CLIENT_ID) + sizeof(CLIENT_SECRET) +
    authorizationCode.length() + accessToken.length() + refreshToken.length() +
    sizeof(accessTokenExpiration) + sizeof(DEVICE_ID);
  DynamicJsonDocument doc(capacity);
  String json;

  doc["clientId"] = CLIENT_ID;
  doc["clientSecret"] = CLIENT_SECRET;
  doc["authorizationCode"] = authorizationCode;
  doc["accessToken"] = accessToken;
  doc["refreshToken"] = refreshToken;
  doc["accessTokenExpiration"] = accessTokenExpiration;
  doc["deviceId"] = DEVICE_ID;

  serializeJson(doc, json);

  server.send(200, "application/json", json);
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
  const size_t capacity = JSON_OBJECT_SIZE(2) + sizeof(long) + sizeof(time_t);
  DynamicJsonDocument doc(capacity);
  String json;

  doc["millis"] = millis();
  doc["time"] = time(nullptr);

  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handle404() {
  server.send(404, "text/html; charset=UTF-8", "<p>dunno sorry 🤷‍♀️</p>");
}

void redirectTo(String url) {
  server.sendHeader("Location", url, true);
  server.send(302, "text/plain", "");
}
