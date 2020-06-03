#include <ArduinoJson.h>
#include <FS.h>
#include "http-server.h"
#include "spotify.h"

ESP8266WebServer server(80);

void handleRoot();
void handleAuth();
void handleAuthCallback();
void handleConfig();
void handleStats();
void handle404();
void redirectTo(String);

void startHttpServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/auth", HTTP_GET, handleAuth);
  server.on("/auth-callback", HTTP_GET, handleAuthCallback);
  server.on("/config", HTTP_GET, handleConfig);
  server.on("/stats", HTTP_GET, handleStats);
  server.onNotFound(handle404);

  server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println("HTTP server started");
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
  const size_t capacity = JSON_OBJECT_SIZE(5) + authorizationCode.length() + accessToken.length() + refreshToken.length() + sizeof(accessTokenExpiration) + sizeof(DEVICE_ID);
  DynamicJsonDocument doc(capacity);
  String json;

  doc["authorizationCode"] = authorizationCode;
  doc["accessToken"] = accessToken;
  doc["refreshToken"] = refreshToken;
  doc["accessTokenExpiration"] = accessTokenExpiration;
  doc["deviceId"] = DEVICE_ID;

  serializeJson(doc, json);

  server.send(200, "application/json", json);
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
