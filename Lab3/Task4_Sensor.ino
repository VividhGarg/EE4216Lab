#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

const char* ssid = "";
const char* password = "";

const char* gatewayURL = "http://192.168.4.1/api/sensor";

#define TRIG_PIN 5
#define ECHO_PIN 18
#define LED_PIN 4
#define NUM_LEDS 8

WebServer server(80);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

unsigned long lastSend = 0;
unsigned long lastReconnect = 0;

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration == 0) return -1;
  return duration * 0.0343f / 2.0f;
}

bool validByte(JsonVariantConst value) {
  return value.is<int>() && value.as<int>() >= 0 && value.as<int>() <= 255;
}

void handleLED() {
  JsonDocument doc;

  if (deserializeJson(doc, server.arg("plain")) || !doc.is<JsonObject>()) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }

  if (!doc["index"].is<int>() || doc["index"].as<int>() < 0 || doc["index"].as<int>() >= NUM_LEDS || !validByte(doc["r"]) || !validByte(doc["g"]) || !validByte(doc["b"])) {
    server.send(400, "application/json", "{\"error\":\"Invalid LED values\"}");
    return;
  }

  int id = doc["index"].as<int>();
  int r = doc["r"].as<int>();
  int g = doc["g"].as<int>();
  int b = doc["b"].as<int>();

  strip.setPixelColor(id, strip.Color(r, g, b));
  strip.show();

  Serial.printf("LED %d: R=%d G=%d B=%d\n", id, r, g, b);

  JsonDocument reply;
  reply["status"] = "ok";
  reply["index"] = id;

  String response;
  serializeJson(reply, response);
  server.send(200, "application/json", response);
}

void sendDistance() {
  if (WiFi.status() != WL_CONNECTED) return;

  float distance = measureDistance();

  if (!isfinite(distance) || distance < 2 || distance > 400) {
    Serial.println("Invalid distance");
    return;
  }

  JsonDocument doc;
  doc["node"] = "sensor01";
  doc["distance"] = distance;

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.setTimeout(3000);

  if (!http.begin(gatewayURL)) return;

  http.addHeader("Content-Type", "application/json");

  int status = http.POST(payload);

  Serial.printf("Distance: %.2f cm | HTTP: %d\n", distance, status);

  http.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  strip.begin();
  strip.clear();
  strip.show();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  server.on("/api/led", HTTP_POST, handleLED);
  server.begin();

  Serial.println("Sensor node started");
}

void loop() {
  server.handleClient();

  if (WiFi.status() != WL_CONNECTED && millis() - lastReconnect >= 5000) {
    lastReconnect = millis();
    WiFi.reconnect();
  }

  if (millis() - lastSend >= 5000) {
    lastSend = millis();
    sendDistance();
  }
}