#include <LittleFS.h>
#include "config.h"
#include "http-server.h"
#include "logger.h"
#include "nfc.h"
#include "wifi.h"

void setup() {
	Serial.begin(115200);
  nfcInit();

  startWifi();

  LittleFS.begin();

  startHttpServer();

  nfcDumpVersionToSerial();

  logger::log(String(F("Free Heap: ")) + String(ESP.getFreeHeap(), DEC));

	logger::log(F("Ready to play some records! 🔊"));
}

void loop() {
  updateDns();
  handleHttpClient();

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

  // TODO: Publish the URL to MQTT

  // Keeps from reading the same card over and over again
  haltNfc();
}
