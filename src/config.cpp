#include <FS.h>
#include "logger.h"

String refreshToken = "";

/**
 * Reads config from file
 */
void initializeConfig() {
  File file = SPIFFS.open("/refreshToken.txt", "r");
  if (!file) {
    logger::log("No saved refresh token.");
    return;
  }

  refreshToken = file.readString();

  logger::log("Read refresh token from file: " + refreshToken);

  file.close();
}

/**
 * Writes config to file
 */
void saveConfig() {
  File file = SPIFFS.open("/refreshToken.txt", "w");

  int bytesWritten = file.print(refreshToken);
  logger::log("💾 Wrote refresh token to file: " + String(bytesWritten, DEC) + " bytes.");

  file.close();
}
