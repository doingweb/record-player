#include "wifi.h"

void startWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);

  Serial.println("");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println((String)"Connected to " + SSID);
  Serial.println((String)"IP address: " + WiFi.localIP().toString());

  if (MDNS.begin(HOSTNAME)) {
    Serial.println("mDNS responder started.");
  }
}

void updateDns() {
  MDNS.update();
}
