#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* apSSID = "";
const char* apPassword = "";
const char* staSSID = "";
const char* staPassword = "";

WebServer server(80);

float latestDistance = -1;
unsigned long receivedAt = 0;
String latestNode = "";

void handleSensor() {
  JsonDocument doc;

  if (deserializeJson(doc, server.arg("plain")) || !doc.is<JsonObject>()) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  JsonVariantConst value = doc["distance"];

  if (!value.is<float>() && !value.is<int>()) {
    server.send(400, "application/json", "{\"error\":\"Invalid distance\"}");
    return;
  }

  float d = value.as<float>();

  if (!isfinite(d) || d < 2 || d > 400) {
    server.send(400, "application/json", "{\"error\":\"Distance out of range\"}");
    return;
  }

  latestDistance = d;
  receivedAt = millis();
  latestNode = doc["node"].as<String>();

  Serial.print("Received distance: ");
  Serial.print(latestDistance);
  Serial.println(" cm");

  Serial.print("Node: ");
  Serial.println(latestNode);

  Serial.print("Received at millis: ");
  Serial.println(receivedAt);

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPassword, 1, 0, 2);
  WiFi.begin(staSSID, staPassword);

  server.on("/api/sensor", HTTP_POST, handleSensor);
  server.begin();

  Serial.print("Gateway AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
}