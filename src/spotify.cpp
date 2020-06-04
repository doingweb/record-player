#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "logger.h"
#include "spotify.h"
#include "../config.h"

// CN: *.spotify.com
const char certSha1Fingerprint[] PROGMEM = "AB BC 7C 9B 7A D8 5D 98 8B B2 72 A4 4C 13 47 9A 00 2F 70 B5";

// The OAuth scopes needed to do stuff
// "user-read-playback-state", "user-modify-playback-state"

WiFiClientSecure getClient();
String encodedRedirectUri();
void updateTokensViaAuthCode();
void refreshAccessToken();
String requestApiTokens(String payload);
void extractTokens(String);
int sendRequest(String, String);
int sendRequest(String, String, String);

// OAuth stuff
String authorizationCode = "";
String accessToken = "";
String refreshToken = ""; // TODO: Write to a file?
int oauthXssState = 0;
time_t accessTokenExpiration = 0;

void playAlbum(String id) {
  logger::log("Playing album " + id + ".");

  const String path = (String)"/v1/me/player/play?device_id=" + DEVICE_ID;
  String payload = "{\"context_uri\":\"spotify:album:" + id + "\"}";

  int status = sendRequest("PUT", path, payload);

  if (status == HTTP_CODE_UNAUTHORIZED || status == HTTP_CODE_BAD_REQUEST) {
    logger::log("Access token did not work. Getting a new one and trying again.");
    refreshAccessToken();
    status = sendRequest("PUT", path, payload);
  }

  // Log if it happens again?
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

  logger::log("Updating access tokens using auth code: " + authorizationCode);

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
  int httpResponseCode = http.POST(payload);
  String response = http.getString();
  http.end();

  logger::log("Token API HTTP status: " + String(httpResponseCode, DEC));

  return response;
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

  logger::log("Received JSON from token API: " + json);

  accessToken = doc["access_token"].isNull() ? "" : doc["access_token"].as<String>();

  if (!doc["refresh_token"].isNull()) {
    refreshToken = doc["refresh_token"].as<String>();
  }

  int expiresIn = doc["expires_in"];

  if (expiresIn == 0) {
    logger::log("No token expiration: Probably something really wrong. Throwing out tokens and starting over.");
    accessToken = "";
    refreshToken = "";
    accessTokenExpiration = 0;
    return;
  }

  accessTokenExpiration = time(nullptr) + expiresIn;
}

int sendRequest(String method, String path) {
  return sendRequest(method, path, "");
}

int sendRequest(String method, String path, String payload) {
  if (accessToken.isEmpty()) {
    logger::log("Cancelling request: No access token set.");
    return -1;
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
  logger::log(String(httpResponseCode, DEC));
  String response = http.getString();
  logger::log(response);

  http.end();

  return httpResponseCode;
}

WiFiClientSecure getClient() {
  WiFiClientSecure client = WiFiClientSecure();
  client.setFingerprint(certSha1Fingerprint);
  return client;
}
