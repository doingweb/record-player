#include <FS.h>
#include "http-server.h"
#include "nfc.h"
#include "recordPlayer.h"
#include "spotify.h"
#include "wifi.h"

void setup() {
	Serial.begin(115200);
  nfcInit();

  startWifi();

  SPIFFS.begin();

  startHttpServer();

  nfcDumpVersionToSerial();

	Serial.println(F("Ready to play some records! 🔊"));
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

	// nfcDumpCardInfoToSerial();

  String uid = getCardUid();
  Serial.println(uid);

  String albumId = getAlbumId();

  if (albumId != "") {
    playAlbum(albumId);
  } else {
    signalUnrecognizedAlbum();
  }

  // Keeps from reading the same card over and over again
  haltNfc();
}
