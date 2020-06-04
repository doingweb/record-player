#include <MFRC522.h>
#include "logger.h"
#include "nfc.h"

#define RST_PIN D1
#define SS_PIN D2

String getAlbumUrlFromCard();
String getAlbumIdFromUrl(String url);
MFRC522::StatusCode readDataFromCard(byte blockAddr, byte *buffer, byte *bufferSize);

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
  String albumUrl = getAlbumUrlFromCard();
  if (albumUrl == "") {
    return "";
  }

  return getAlbumIdFromUrl(albumUrl);
}

String getAlbumUrlFromCard() {
  /*
  Example NDEF Message read

  Skip the first 4 pages, since those are metadata about the card (UID etc.).

  03 4C D1 01 48 55 04 6F 70 65 6E 2E 73 70 6F 74
  69 66 79 2E 63 6F 6D 2F 61 6C 62 75 6D 2F 31 54
  34 6D 7A 6B 4F 56 50 39 30 32 4D 43 36 66 63 30
  51 63 6D 73 3F 73 69 3D 6A 34 31 64 43 65 6B 4B
  54 43 36 63 63 55 66 55 4E 33 48 6F 34 51 FE 00

  ....HU.open.spotify.com/album/1T4mzkOVP902MC6fc0Qcms?si=j41dCekKTC6ccUfUN3Ho4Q..

  Album is Funeral by The Arcade Fire.

  Organized as a "TLV" block - Type, Length, Value.

  0x03 - Type - Signals beginning of "NDEF Message"-type block
  0x4C - Length - Number of bytes in the block, not including the terminator byte
  Now the block starts with a header:
    0xD1 - NDEF Record Header - flags: 0b1101 + TNF 0x01 ("Well-Known Record").
    0x01 - Type Length - URI, represented by the single byte 0x55
    0x48 - Payload Length - 0x47 bytes of URL + the URI Identifier at the beginning
    0x55 - Record Type Indicator - 'U', meaning "URI"
  Now the Payload:
    0x04 - URI Identifier - "https://"
    Then the rest of the payload. Continue until the record's Payload Length is consumed
  0xFE - Terminator of the block
  */

  // Only supporing the sticker kind for now ("MIFARE Ultralight or Ultralight C" - NTAG215)
  // Also note that this doesn't really follow the spec. Since for our application
  //   we're only storing one record of the type URI, we're taking a few shortcuts.
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_UL) {
    logger::log("Unknown card type.");
    return "";
  }

  MFRC522::StatusCode status;
  byte currentPageset = 1; // We read 4 pages at a time (a "pageset"). User data space begins on page 4, so skip pageset 0.
	byte buffer[18]; // 16 bytes (4 pages) + 2 bytes CRC_A
	byte bufferSize = sizeof(buffer);
	byte bufferIndex;

  String url = "";

  status = readDataFromCard(currentPageset, buffer, &bufferSize);
  if (status != MFRC522::STATUS_OK) {
    return "";
  }

  // Make sure the message and record headers are exactly as we expect them
  if (!(
    buffer[0] == 0x03 && // Must be an NDEF Message block
    // (Ignoring block size, since we only care about the first record)
    buffer[2] == 0xD1 && // NDEF Record Header should always be the same
    buffer[3] == 0x01 && // Type should be URI, which is 1 byte
    // We'll come back for the Payload Length
    buffer[5] == 0x55 && // 'U' for URI
    buffer[6] == 0x04 // Should be an https:// URI
  )) {
    return "";
  }

  const byte payloadLength = buffer[4];
  byte payloadConsumed = 1; // The 0x04 was technically part of the payload, so count it as consumed
  bufferIndex = 7; // Start after the 0x04

  while (payloadConsumed < payloadLength) {
    // Buffer the next bytes and go back to the start
    if (bufferIndex >= bufferSize - 2) { // 2 bytes of checksum, '=' for comparing an index against a size
      currentPageset++;
      status = readDataFromCard(currentPageset, buffer, &bufferSize);
      if (status != MFRC522::STATUS_OK) {
        return url;
      }
      bufferIndex = 0;
    }

    url.concat((char)buffer[bufferIndex]);
    payloadConsumed++;
    bufferIndex++;
  }

  return url;
}

String getAlbumIdFromUrl(String url) {
  // TODO: Implement
  logger::log("Got album URL: \"" + url + "\"");
  return "";
}

MFRC522::StatusCode readDataFromCard(byte pageset, byte *buffer, byte *bufferSize) {
  byte blockAddr = pageset * 4; // Read 4 pages at a time

  MFRC522::StatusCode status = mfrc522.MIFARE_Read(blockAddr, buffer, bufferSize);

  if (status != MFRC522::STATUS_OK) {
    logger::log(String(F("MIFARE_Read() failed: ")) + MFRC522::GetStatusCodeName(status));
  }

  return status;
}
