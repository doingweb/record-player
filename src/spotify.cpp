#include "spotify.h"

// CN: *.spotify.com
const char certSha1Fingerprint[] PROGMEM = "AB BC 7C 9B 7A D8 5D 98 8B B2 72 A4 4C 13 47 9A 00 2F 70 B5";

// The OAuth scopes needed to do stuff
// "user-read-playback-state", "user-modify-playback-state"

WiFiClientSecure getClient();
String encodedRedirectUri();
void updateTokensViaAuthCode();
void updateTokensViaRefresh();
String requestApiTokens(String payload);
void extractTokens(String);
void sendRequest(String, String);
void sendRequest(String, String, String);

// OAuth stuff
String authorizationCode = ""; // TODO: Write to a file?
String accessToken = "";
String refreshToken = "";
int oauthXssState;

void playAlbum(String id) {
  Serial.println("Playing " + id);

  const String path = (String)"/v1/me/player/play?device_id=" + DEVICE_ID;
  String payload = "{\"context_uri\":\"spotify:album:" + id + "\"}";

  sendRequest("PUT", path, payload);
}

String getAuthorizeUrl() {
  oauthXssState = random(10000);
  return (String)"https://accounts.spotify.com/authorize?client_id=" + CLIENT_ID + "&response_type=code&redirect_uri=" + encodedRedirectUri() + "&scope=user-read-playback-state%20user-modify-playback-state&state=" + String(oauthXssState);
}

String encodedRedirectUri() {
  return (String)"http%3A%2F%2F" + HOSTNAME + ".local%2Fauth-callback";
}

void updateAccessToken() {
  if (authorizationCode == "" && refreshToken == "") {
    Serial.println("Can't update access tokens because we lack auth code or refresh token");
    return;
  }

  if (refreshToken != "") {
    Serial.println("Updating access tokens via a refresh");
    updateTokensViaRefresh();
    return;
  }

  if (authorizationCode != "") {
    Serial.println("Updating access tokens via auth code");
    updateTokensViaAuthCode();
    return;
  }
}

void updateTokensViaAuthCode() {
  String payload = "grant_type=authorization_code&code=" + authorizationCode + "&redirect_uri=" + encodedRedirectUri();
  extractTokens(requestApiTokens(payload));
}

void updateTokensViaRefresh() {
  String payload = "grant_type=refresh_token&refresh_token=" + refreshToken;
  extractTokens(requestApiTokens(payload));
}

String requestApiTokens(String payload) {
  HTTPClient http;
  WiFiClientSecure client = getClient();

  http.begin(client, "https://accounts.spotify.com/api/token");
  http.setAuthorization(CLIENT_ID, CLIENT_SECRET);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(payload);
  String response = http.getString();
  http.end();

  return response;
}

/**
 * Finds tokens in the supplied raw JSON and sets our state to use them.
 */
void extractTokens(String json) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, json);

  accessToken = doc["access_token"].as<String>();

  if (doc["refresh_token"] != "") {
    refreshToken = doc["refresh_token"].as<String>();
  }
}

void sendRequest(String method, String path) {
  sendRequest(method, path, "");
}

void sendRequest(String method, String path, String payload) {
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
  Serial.println(httpResponseCode);
  String response = http.getString();
  Serial.println(response);

  // TODO: Refresh access token and retry if it's expired
  // if (httpResponseCode == HTTP_CODE_UNAUTHORIZED) {}

  http.end();
}

WiFiClientSecure getClient() {
  WiFiClientSecure client = WiFiClientSecure();
  client.setFingerprint(certSha1Fingerprint);
  return client;
}
