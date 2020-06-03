#include "Arduino.h"

extern String authorizationCode;
extern String accessToken;
extern String refreshToken;
extern time_t accessTokenExpiration;

void playAlbum(String id);

String getAuthorizeUrl();
void receiveAuthCode(String code, String state);

// To be called in the loop to make sure the access token gets refreshed when it expires.
void maintainAccessToken();
