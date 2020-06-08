#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include "logger.h"
#include "spotify.h"
#include "config.h"
#include "../config.h"

// Tried to issue an API request without an access token
#define API_REQUEST_CANCELED_NO_TOKEN -100

// CN: *.spotify.com
const char certSha1Fingerprint[] PROGMEM = "AB BC 7C 9B 7A D8 5D 98 8B B2 72 A4 4C 13 47 9A 00 2F 70 B5";

// The OAuth scopes needed to do stuff
// "user-read-playback-state", "user-modify-playback-state"

struct httpResponse
{
  int statusCode;
  String body;
};

WiFiClientSecure getClient();
String encodedRedirectUri();
void updateTokensViaAuthCode();
void refreshAccessToken();
String requestApiTokens(String payload);
void extractTokens(String);
void clearTokens();
httpResponse sendApiRequest(String, String);
httpResponse sendApiRequest(String, String, String);

// OAuth stuff
String authorizationCode = "";
String accessToken = "";
int oauthXssState = 0;
time_t accessTokenExpiration = 0;

bool isAlreadyPlaying(String id) {
  // https://api.spotify.com/v1/me/player/currently-playing
  // https://developer.spotify.com/console/get-users-currently-playing-track/
  // Something like `responseJson.context.uri == "spotify:album:" + albumId`?
  httpResponse response = sendApiRequest("GET", "/v1/me/player/currently-playing");

  if (response.statusCode == HTTP_CODE_NO_CONTENT) {
    // Nothing is playing
    return false;
  }

  if (response.statusCode != HTTP_CODE_OK) {
    logger::log("Not sure if it's already playing... Let's assume not.");
    return false;
  }

  DynamicJsonDocument doc(6144);
  deserializeJson(doc, response.body);

  Serial.println("response.body: " + response.body);
  Serial.println("Currently playing doc is taking up: " + String(doc.memoryUsage(), DEC));

  if (!doc["is_playing"]) {
    // There's a context, but it's paused
    Serial.println(F("There's a context, but it's paused."));
    return false;
  }

  String contextUri = !doc["context"].isNull() && !doc["context"]["uri"].isNull() ? doc["context"]["uri"].as<String>() : "";
  if (contextUri != "") {
    logger::log("Currently playing context URI: " + contextUri);
  } else {
    logger::log("Context was empty");
  }

  return contextUri == "spotify:album:" + id;
}

void playAlbum(String id) {
  const String path = (String)"/v1/me/player/play?device_id=" + DEVICE_ID;
  String payload = "{\"context_uri\":\"spotify:album:" + id + "\"}";

  httpResponse response = sendApiRequest("PUT", path, payload);

  if (response.statusCode == HTTP_CODE_UNAUTHORIZED) {
    logger::log("Access token did not work. Getting a new one and trying again.");
    refreshAccessToken();
    response = sendApiRequest("PUT", path, payload);
    // Log if it happens again?
  }

  if (response.statusCode == HTTP_CODE_NO_CONTENT) {
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

httpResponse sendApiRequest(String method, String path) {
  return sendApiRequest(method, path, "");
}


httpResponse sendApiRequest(String method, String path, String payload) {
  const size_t sslErrorSize = 500;
  char sslError[sslErrorSize]; // Longest in WiFiClientSecureBearSSL.cpp is 315.
  httpResponse response;

  if (accessToken.isEmpty()) {
    logger::log("Cancelling API request: No access token set.");
    response.statusCode = API_REQUEST_CANCELED_NO_TOKEN;
    return response;
  }

  HTTPClient http;
  WiFiClientSecure client = getClient();

  String url = "https://api.spotify.com" + path;

  http.begin(client, url);
  http.addHeader("Accept", "application/json");
  http.addHeader("Connection", "close");
  http.addHeader("Authorization", (String)"Bearer " + accessToken);

  if (!payload.isEmpty()) {
    http.addHeader("Content-Type", "application/json");
  }

  // Send the request
  int httpResponseCode = http.sendRequest(method.c_str(), payload);

  // Barf it out for debugging purposes
  logger::log(String(httpResponseCode, DEC) + F(" 🌎 ") + method + F(" ") + path);
  if (httpResponseCode < 0) {
    logger::log("⛔️ HTTP Error: " + HTTPClient::errorToString(httpResponseCode));

    int sslErrorCode = client.getLastSSLError(sslError, sslErrorSize);
    logger::log("🔒 SSL Error (?) " + String(sslErrorCode, DEC) + F(": ") + sslError);
  }

  String responseBody = http.getString();
  // if (!responseBody.isEmpty()) {
  //   logger::log(responseBody);
  // }

  http.end();

  response.statusCode = httpResponseCode;
  response.body = responseBody;

  return response;
}

WiFiClientSecure getClient() {
  WiFiClientSecure client = WiFiClientSecure();
  client.setFingerprint(certSha1Fingerprint);
  return client;
}
