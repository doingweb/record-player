#include <FS.h>
#include "config.h"
#include "http-server.h"
#include "logger.h"
#include "nfc.h"
#include "spotify.h"
#include "wifi.h"

void setup() {
	Serial.begin(115200);
  nfcInit();

  startWifi();

  SPIFFS.begin();

  startHttpServer();

  nfcDumpVersionToSerial();

  logger::log(String(F("Free Heap: ")) + String(ESP.getFreeHeap(), DEC));

  initializeConfig();
  if (refreshToken != "") {
    refreshAccessToken();
  }

	logger::log(F("Ready to play some records! 🔊"));
}

void loop() {
  updateDns();
  handleHttpClient();
  maintainAccessToken();

	if (!isNewCardPresent()) {
		return;
	}

	if (!readCard()) {
		return;
	}

  String uid = getCardUid();
  logger::log(String(F("💿 Scanned ")) + uid);

  String albumId = getAlbumId();

  if (albumId != "") {
    if (isAlreadyPlaying(albumId)) {
      logger::log("This album is already playing.");
    } else {
      playAlbum(albumId);
    }
  } else {
    logger::log(F("⏹ Not playing: Unable to determine album ID."));
  }

  // Keeps from reading the same card over and over again
  haltNfc();
}
