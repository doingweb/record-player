#include "nfc.h"

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void nfcInit() {
  SPI.begin();
  mfrc522.PCD_Init();
}

void nfcDumpVersionToSerial() {
	mfrc522.PCD_DumpVersionToSerial();
}

void nfcDumpCardInfoToSerial() {
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
}

bool isNewCardPresent() {
  return mfrc522.PICC_IsNewCardPresent();
}

bool readCard() {
  return mfrc522.PICC_ReadCardSerial();
}

void haltNfc() {
  mfrc522.PICC_HaltA();
}

// TODO: We really need this?
/**
 * Convert UID read from card to printable string
 */
String getUidString(const byte *uidBytes, size_t size) {
  String content = "";

  for (byte i = 0; i < size; i++) {
    content.concat(String(uidBytes[i] < 0x10 ? " 0" : " "));
    content.concat(String(uidBytes[i], HEX));
  }

  content.toUpperCase();

  return content.substring(1);
}

String getCardUid() {
  return getUidString(mfrc522.uid.uidByte, mfrc522.uid.size);
}

String getAlbumId() {
  // TODO: Implement for real
  return "";
}
