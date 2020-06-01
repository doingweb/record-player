#include <ESP8266WiFi.h>
#include "Arduino.h"
#include "../config.h"

const char certSha1Fingerprint[] PROGMEM = "AB BC 7C 9B 7A D8 5D 98 8B B2 72 A4 4C 13 47 9A 00 2F 70 B5";

// The scopes needed to do our thing. May need changing
const char scopes[] PROGMEM = "user-modify-playback-state";

void playAlbum(String id);
