#include <MFRC522.h>
#include <SPI.h>

#define RST_PIN D1
#define SS_PIN D2

void nfcInit();
void nfcDumpVersionToSerial();
void nfcDumpCardInfoToSerial();
bool isNewCardPresent();
bool readCard();
void haltNfc();

String getUidString(const byte *, size_t);
String getCardUid();
String getAlbumId();
