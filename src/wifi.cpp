#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include "Arduino.h"
#include "logger.h"
#include "wifi.h"
#include "../config.h"

void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  logger::log("Connected to " + String(WIFI_SSID));
  logger::log("IP address: " + WiFi.localIP().toString());

  if (MDNS.begin(HOSTNAME)) {
    logger::log("mDNS responder started.");
  }
}

void updateDns() {
  MDNS.update();
}
