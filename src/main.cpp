#include <LittleFS.h>
#include "config.h"
#include "http-server.h"
#include "logger.h"
#include "mqtt.h"
#include "nfc.h"
#include "wifi.h"
#include "../config.h"

void setClock();

void setup() {
	Serial.begin(115200);
  nfcInit();

  startWifi();

  setClock(); // Required for TLS

  LittleFS.begin();

  startHttpServer();

  nfcDumpVersionToSerial();

  mqtt::setup();

  logger::log(String(F("Free Heap: ")) + String(ESP.getFreeHeap(), DEC));

	logger::log(F("Ready to play some records! 🔊"));
}

void loop() {
  updateDns();
  handleHttpClient();
  mqtt::loop();

	if (!isNewCardPresent()) {
		return;
	}

	if (!readCard()) {
		return;
	}

  String uid = getCardUid();
  logger::log(String(F("💿 Scanned ")) + uid);

  String url = getUrlFromCard();
  if (url == "") {
    logger::log(F("Unable to get URL from card."));
    haltNfc();
    return;
  }

  logger::log("Found URL: \"" + url + F("\"."));

  mqtt::publish(url.c_str());

  logger::log(F("▶️ Published to MQTT!"));

  // Keeps from reading the same card over and over again
  haltNfc();
}

void setClock() {
  configTime(TZ, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");

  struct tm timeinfo;

  localtime_r(&now, &timeinfo);
  Serial.print("Current local time: ");
  Serial.print(asctime(&timeinfo));
}
