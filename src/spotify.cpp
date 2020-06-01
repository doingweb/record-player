#include "spotify.h"

void playAlbum(String id) {
  Serial.println("Playing " + id);

  WiFiClientSecure client = WiFiClientSecure();

  client.setFingerprint(certSha1Fingerprint);

  String apiHost = "api.spotify.com";

  if (!client.connect(apiHost.c_str(), 443)) {
    Serial.println("oof no connection ☹️");
    return;
  }

  String payload = "{\"context_uri\":\"spotify:album:" + id + "\"}";

  // Send the request
  client.println((String)"PUT /v1/me/player/play?device_id=" + DEVICE_ID + " HTTP/1.1");
  client.println("Host: " + apiHost);
  client.println("Accept: application/json");
  client.println("Content-Type: application/json");
  client.println((String)"Authorization: Bearer " + BEARER_TOKEN);
  client.println("Content-Length: " + String(payload.length()));
  client.println();
  client.println(payload);
  client.println();

  // Wait for it to come back
  int retries = 0;
  while(!client.available()) {
    retries++;
    if (retries > 10) {
      return;
    }

    delay(100);
  }

  // Barf it out for debugging purposes
  while(client.available()) {
    char c = client.read();
    Serial.print(c);
  }
  Serial.println();
}
