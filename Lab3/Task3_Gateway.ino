#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <math.h>

const char* apSSID = "";
const char* apPassword = "";

const char* staSSID = "";
const char* staPassword = "";

const char* writeAPIKey = "";

WebServer server(80);

float latestDistance = -1;
bool hasReading = false;
unsigned long receivedAt = 0;
unsigned long lastUpload = 0;

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

  float distance = value.as<float>();

  if (!isfinite(distance) || distance < 2 || distance > 400) {
    server.send(400, "application/json", "{\"error\":\"Distance out of range\"}");
    return;
  }

  latestDistance = distance;
  receivedAt = millis();
  hasReading = true;

  Serial.print("Received distance: ");
  Serial.print(latestDistance, 2);
  Serial.println(" cm");

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void uploadToThingSpeak() {
  if (WiFi.status() != WL_CONNECTED || !hasReading) return;

  if (millis() - receivedAt > 15000) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(5000);

  if (!http.begin(client, "https://api.thingspeak.com/update")) {
    Serial.println("HTTPS connection failed");
    return;
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String payload = "api_key=" + String(writeAPIKey) + "&field1=" + String(latestDistance, 2);

  int status = http.POST(payload);
  String response = http.getString();
  response.trim();

  Serial.print("ThingSpeak HTTP: ");
  Serial.println(status);

  Serial.print("ThingSpeak response: ");
  Serial.println(response);

  if (status == 200 && response.toInt() > 0) {
    Serial.println("Upload successful");
  } else {
    Serial.println("Upload failed");
  }

  http.end();
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apSSID, apPassword, 1, 0, 2);
  WiFi.begin(staSSID, staPassword);
  WiFi.setAutoReconnect(true);

  server.on("/api/sensor", HTTP_POST, handleSensor);
  server.begin();

  Serial.print("Gateway AP IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();

  static unsigned long lastReconnect = 0;

  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastReconnect >= 5000) {
    lastReconnect = millis();
    WiFi.reconnect();
  }

  static bool printedIP = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!printedIP) {
      Serial.print("Gateway STA IP: ");
      Serial.println(WiFi.localIP());
      printedIP = true;
    }
  } else {
    printedIP = false;
  }

  if (millis() - lastUpload >= 20000) {
    lastUpload = millis();
    uploadToThingSpeak();
  }
}