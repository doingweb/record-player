void nfcInit();
void nfcDumpVersionToSerial();
void nfcDumpCardInfoToSerial();
bool isNewCardPresent();
bool readCard();
void haltNfc();

String getUidString(const byte *, size_t);
String getCardUid();
String getAlbumId();
