#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "logger.h"
#include "spotify.h"
#include "config.h"
#include "../config.h"

// Tried to issue an API request without an access token
#define API_REQUEST_CANCELED_NO_TOKEN -1

// CN: *.spotify.com
const char certSha1Fingerprint[] PROGMEM = "B9 79 6B CE FD 61 21 97 A7 02 90 EE DA CD F0 A0 44 13 0E EB";

// The OAuth scopes needed to do stuff
// "user-read-playback-state", "user-modify-playback-state"

WiFiClientSecure getClient();
String encodedRedirectUri();
void updateTokensViaAuthCode();
void refreshAccessToken();
String requestApiTokens(String payload);
void extractTokens(String);
void clearTokens();
int sendRequest(String, String);
int sendRequest(String, String, String);

// OAuth stuff
String authorizationCode = "";
String accessToken = "";
int oauthXssState = 0;
time_t accessTokenExpiration = 0;

void playAlbum(String id) {
  const String path = (String)"/v1/me/player/play?device_id=" + DEVICE_ID;
  String payload = "{\"context_uri\":\"spotify:album:" + id + "\"}";

  int status = sendRequest("PUT", path, payload);

  if (status == HTTP_CODE_UNAUTHORIZED) {
    logger::log("Access token did not work. Getting a new one and trying again.");
    refreshAccessToken();
    status = sendRequest("PUT", path, payload);
    // Log if it happens again?
  }

  if (status == HTTP_CODE_NO_CONTENT) {
    logger::log(F("▶️ Playing album!"));
  }
}

String getAuthorizeUrl() {
  oauthXssState = random(1, 10000);
  return String("https://accounts.spotify.com/authorize?client_id=") + CLIENT_ID +
    "&response_type=code&redirect_uri=" + encodedRedirectUri() +
    "&scope=user-read-playback-state%20user-modify-playback-state&state=" + oauthXssState;
}

void receiveAuthCode(String code, String state) {
  if (state.toInt() != oauthXssState) {
    return;
  }
  oauthXssState = 0; // Use it once and dispose

  if (code.isEmpty()) {
    return;
  }

  authorizationCode = code;

  updateTokensViaAuthCode();
}

void maintainAccessToken() {
  if (accessTokenExpiration == 0) {
    return;
  }

  time_t currentTime = time(nullptr);
  if (currentTime > accessTokenExpiration) {
    logger::log("Access token expired! Refreshing...");
    accessTokenExpiration = 0;
    refreshAccessToken();
  }
}

String encodedRedirectUri() {
  return (String)"http%3A%2F%2F" + HOSTNAME + ".local%2Fauth-callback";
}

void updateTokensViaAuthCode() {
  if (authorizationCode.isEmpty()) {
    logger::log("Can't get access token: No auth code.");
    return;
  }

  logger::log("Using auth code to get access tokens.");

  String payload = "grant_type=authorization_code&code=" + authorizationCode + "&redirect_uri=" + encodedRedirectUri();
  extractTokens(requestApiTokens(payload));
}

void refreshAccessToken() {
  if (refreshToken.isEmpty()) {
    logger::log("Can't refresh access token: No refresh token.");
    return;
  }

  logger::log("Refreshing access token using refresh token: " + refreshToken);

  String payload = "grant_type=refresh_token&refresh_token=" + refreshToken;
  extractTokens(requestApiTokens(payload));
}

String requestApiTokens(String payload) {
  HTTPClient http;
  WiFiClientSecure client = getClient();

  http.begin(client, "https://accounts.spotify.com/api/token");
  http.setAuthorization(CLIENT_ID, CLIENT_SECRET);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Connection", "close");
  int httpResponseCode = http.POST(payload);
  String responseBody = http.getString();
  http.end();

  logger::log(String(httpResponseCode, DEC) + F(" 🌎 POST /api/token"));
  if (httpResponseCode < 0) {
    logger::log(String(F("⛔️ ")) + HTTPClient::errorToString(httpResponseCode));
  }

  return responseBody;
}

/**
 * Finds tokens in the supplied raw JSON and sets our state to use them.
 */
void extractTokens(String json) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, json);

  if (doc.containsKey("error")) {
    logger::log("Error extracting tokens: " + json);
    return;
  }

  logger::log("🔑 " + json);

  accessToken = doc["access_token"].isNull() ? "" : doc["access_token"].as<String>();

  if (!doc["refresh_token"].isNull()) {
    refreshToken = doc["refresh_token"].as<String>();
    saveConfig();
  }

  int expiresIn = doc["expires_in"];

  if (expiresIn == 0) {
    logger::log("😨 No token expiration: Probably something really wrong. Clearing tokens. Please reauthorize.");
    clearTokens();
    return;
  }

  accessTokenExpiration = time(nullptr) + expiresIn;
}

void clearTokens() {
  accessToken = "";
  accessTokenExpiration = 0;
  authorizationCode = "";
  refreshToken = "";
  saveConfig();
}

int sendRequest(String method, String path) {
  return sendRequest(method, path, "");
}

int sendRequest(String method, String path, String payload) {
  if (accessToken.isEmpty()) {
    logger::log("Cancelling API request: No access token set.");
    return API_REQUEST_CANCELED_NO_TOKEN;
  }

  HTTPClient http;
  WiFiClientSecure client = getClient();

  String url = "https://api.spotify.com" + path;

  http.begin(client, url);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");
  http.addHeader("Authorization", (String)"Bearer " + accessToken);
  http.addHeader("Content-Length", String(payload.length()));

  // Send the request
  int httpResponseCode = http.sendRequest(method.c_str(), payload);

  // Barf it out for debugging purposes
  logger::log(String(httpResponseCode, DEC) + F(" 🌎 ") + method + F(" ") + path);
  String responseBody = http.getString();
  if (!responseBody.isEmpty()) {
    logger::log(responseBody);
  }

  http.end();

  return httpResponseCode;
}

WiFiClientSecure getClient() {
  WiFiClientSecure client = WiFiClientSecure();
  client.setFingerprint(certSha1Fingerprint);
  return client;
}
