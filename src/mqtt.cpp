#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "logger.h"
#include "../config.h"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void reconnect();

namespace mqtt {
  void setup() {
    mqttClient.setServer(MQTT_BROKER, 1883);
  }

  void loop() {
    if (!mqttClient.connected()) {
      reconnect();
    }
    mqttClient.loop();
  }

  void publish(const char *message) {
    mqttClient.publish(MQTT_TOPIC, message);
  }
}

void reconnect() {
  logger::log(F("Attempting MQTT connection..."));
  const char* clientId = "record-player";

  while (!mqttClient.connected()) {
    if (mqttClient.connect(clientId)) {
      break;
    }

    logger::log(
      String(F("Connect failed with state ")) +
      String(mqttClient.state(), DEC) +
      String(F(" - trying again in 5 seconds"))
    );

    delay(5000);
  }

  logger::log(F("Connected!"));
}
