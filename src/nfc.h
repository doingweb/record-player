void nfcInit();
void nfcDumpVersionToSerial();
void nfcDumpCardInfoToSerial();
bool isNewCardPresent();
bool readCard();
void haltNfc();

String getCardUid();
String getUrlFromCard();
