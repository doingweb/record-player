#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "logger.h"
#include "../config.h"

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

static const char ca_cert[] PROGMEM = CA_CERT;
X509List certs(ca_cert);

void reconnectMqtt();

namespace mqtt {
  void setup() {
    wifiClient.setTrustAnchors(&certs);
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  }

  void loop() {
    if (!mqttClient.connected()) {
      reconnectMqtt();
    }

    mqttClient.loop();
  }

  void publish(const char *message) {
    mqttClient.publish(MQTT_TOPIC, message);
  }
}

void reconnectMqtt() {
  logger::log(F("Attempting MQTT connection..."));

  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      break;
    }

    logger::log(
      String(F("Connect failed. MQTT client state: ")) +
      String(mqttClient.state(), DEC) +
      String(F(" - trying again in 5 seconds"))
    );

    delay(5000);
  }

  logger::log(F("Connected!"));
}
