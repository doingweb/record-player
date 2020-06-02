#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "Arduino.h"
#include "../config.h"

extern String authorizationCode;
extern String accessToken;
extern String refreshToken;
extern int oauthXssState;

void playAlbum(String id);

String getAuthorizeUrl();
void updateAccessToken();
