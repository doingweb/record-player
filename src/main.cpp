#include <FS.h>
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
  logger::log(uid);

  String albumId = getAlbumId();

  if (albumId != "") {
    playAlbum(albumId);
  } else {
    logger::log("Unrecognized album.");
  }

  // Keeps from reading the same card over and over again
  haltNfc();
}
