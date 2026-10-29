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

const char* sensorWriteKey = "";
const char* ledChannelID = "";
const char* ledReadKey = "";

WebServer server(80);

float latestDistance = -1;
unsigned long receivedAt = 0;
bool hasReading = false;

IPAddress sensorIP;
bool sensorKnown = false;

long lastCompletedEntry = 0;

unsigned long lastUpload = 0;
unsigned long lastPoll = 0;
unsigned long lastReconnect = 0;

void handleSensor() {
  JsonDocument doc;

  if (deserializeJson(doc, server.arg("plain")) || !doc.is<JsonObject>()) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  JsonVariantConst value = doc["distance"];

  if ((!value.is<float>() && !value.is<int>()) || !isfinite(value.as<float>()) || value.as<float>() < 2 || value.as<float>() > 400) {
    server.send(400, "application/json", "{\"error\":\"Invalid distance\"}");
    return;
  }

  latestDistance = value.as<float>();
  receivedAt = millis();
  hasReading = true;

  sensorIP = server.client().remoteIP();
  sensorKnown = true;

  Serial.printf("Received: %.2f cm from %s\n", latestDistance, sensorIP.toString().c_str());

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void uploadDistance() {
  if (WiFi.status() != WL_CONNECTED || !hasReading) return;
  if (millis() - receivedAt > 15000) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setTimeout(5000);

  if (!http.begin(client, "https://api.thingspeak.com/update"))
    return;

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String payload = "api_key=" + String(sensorWriteKey) + "&field1=" + String(latestDistance, 2);

  int status = http.POST(payload);
  String response = http.getString();
  response.trim();

  Serial.printf("ThingSpeak upload: HTTP %d | Entry: %s\n", status, response.c_str());

  http.end();
}

bool forwardLED(int id, int r, int g, int b) {
  if (!sensorKnown) {
    Serial.println("Sensor IP not known yet");
    return false;
  }

  String url = "http://" + sensorIP.toString() + "/api/led";

  HTTPClient http;
  http.setTimeout(3000);

  if (!http.begin(url)) return false;

  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["index"] = id;
  doc["r"] = r;
  doc["g"] = g;
  doc["b"] = b;

  String payload;
  serializeJson(doc, payload);

  int status = http.POST(payload);
  String response = http.getString();

  http.end();

  if (status != 200) {
    Serial.printf("LED forwarding failed: %d\n", status);
    return false;
  }

  JsonDocument ack;

  if (deserializeJson(ack, response) || ack["status"] != "ok" || !ack["index"].is<int>() || ack["index"].as<int>() != id) {
    Serial.println("Invalid acknowledgment");
    return false;
  }

  return true;
}

void pollLED() {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  String url = "https://api.thingspeak.com/channels/" + String(ledChannelID) + "/feeds/last.json?api_key=" + String(ledReadKey);

  HTTPClient http;
  http.setTimeout(5000);

  if (!http.begin(client, url)) return;

  int status = http.GET();
  String response = http.getString();
  http.end();

  if (status != 200) {
    Serial.printf("LED poll failed: %d\n", status);
    return;
  }

  JsonDocument doc;

  if (deserializeJson(doc, response) || !doc.is<JsonObject>()) {
    Serial.println("Invalid cloud JSON");
    return;
  }

  long entry = doc["entry_id"] | 0L;
  String indexText = doc["field1"].as<String>();
  String colorText = doc["field2"].as<String>();

  if (entry <= 0 || indexText.length() != 1 || colorText.length() != 1 || indexText[0] < '0' || indexText[0] > '7' || colorText[0] < '0' || colorText[0] > '3') {
    Serial.println("Invalid cloud command");
    return;
  }

  if (entry <= lastCompletedEntry) return;

  int id = indexText.toInt();
  int color = colorText.toInt();

  int r = 0, g = 0, b = 0;

  if (color == 1) r = 255;
  else if (color == 2) g = 255;
  else if (color == 3) b = 255;

  Serial.printf("Command: entry=%ld LED=%d color=%d\n", entry, id, color);

  if (forwardLED(id, r, g, b)) {
    lastCompletedEntry = entry;
    Serial.println("LED updated successfully");
  } else {
    Serial.println("Command pending, will retry");
  }
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

  if (WiFi.status() != WL_CONNECTED && millis() - lastReconnect >= 5000) {
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
    uploadDistance();
  }

  if (millis() - lastPoll >= 5000) {
    lastPoll = millis();
    pollLED();
  }
}