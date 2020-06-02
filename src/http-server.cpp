#include <FS.h>
#include "http-server.h"

ESP8266WebServer server(80);

void handleRoot();
void handleAuth();
void handle404();

void startHttpServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/auth", HTTP_GET, handleAuth);
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
  // TODO: Act on authorization code
  server.send(200, "text/html", "<p>Auth page.</p>");
}

void handle404() {
  server.send(404, "text/html; charset=UTF-8", "<p>dunno sorry 🤷‍♀️</p>");
}
