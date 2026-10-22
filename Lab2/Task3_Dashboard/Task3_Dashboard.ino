#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include "lab_page.h"

const char* apSSID = "A0284640A";
const char* apPassword = "HelloWorld";

#define TRIG_PIN 5
#define ECHO_PIN 18
#define LED_PIN 4
#define NUM_LEDS 8

WebServer server(80);

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

struct LEDState {
  int r;
  int g;
  int b;
  int brightness;
};

LEDState leds[NUM_LEDS] = {};

void sendJSON(int status, JsonDocument& doc) {
  String response;
  serializeJson(doc, response);

  server.send(status, "application/json", response);
}

void sendError(int status, const char* message) {
  JsonDocument doc;
  doc["error"] = message;

  sendJSON(status, doc);
}

void addLEDState(JsonObject obj, int id) {
  obj["id"] = id;
  obj["r"] = leds[id].r;
  obj["g"] = leds[id].g;
  obj["b"] = leds[id].b;
  obj["brightness"] = leds[id].brightness;
}

void updateStrip() {
  for (int i = 0; i < NUM_LEDS; i++) {

    int r = leds[i].r * leds[i].brightness / 255;
    int g = leds[i].g * leds[i].brightness / 255;
    int b = leds[i].b * leds[i].brightness / 255;

    strip.setPixelColor(i, strip.Color(r, g, b));
  }

  strip.show();
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);

  if (duration == 0) {
    return -1.0f;
  }

  return duration * 0.0343f / 2.0f;
}

void handleRoot() {
  server.send_P(200, "text/html", LAB_PAGE);
}

void handleDistance() {
  float distance = measureDistance();

  JsonDocument doc;

  if (distance < 0.0f) {
    doc["distance_cm"] = nullptr;
    doc["valid"] = false;
  } else {
    doc["distance_cm"] = distance;
    doc["valid"] = true;
  }

  server.sendHeader("Cache-Control", "no-store");

  sendJSON(200, doc);
}

void handleGetLEDs() {
  JsonDocument doc;

  JsonArray array = doc["leds"].to<JsonArray>();

  for (int i = 0; i < NUM_LEDS; i++) {
    JsonObject obj = array.add<JsonObject>();
    addLEDState(obj, i);
  }

  sendJSON(200, doc);
}

bool validByte(JsonVariantConst value) {
  if (!value.is<int>()) {
    return false;
  }

  int number = value.as<int>();

  return number >= 0 && number <= 255;
}

void handlePutLED(int id) {

  String body = server.arg("plain");

  JsonDocument request;

  DeserializationError error = deserializeJson(request, body);

  if (error || !request.is<JsonObject>()) {
    sendError(400, "Invalid JSON body");
    return;
  }

  JsonObjectConst obj = request.as<JsonObjectConst>();

  if (!validByte(obj["r"]) || !validByte(obj["g"]) || !validByte(obj["b"]) || !validByte(obj["brightness"])) {

    sendError(400, "RGB and brightness must be integers from 0 to 255");
    return;
  }

  leds[id].r = obj["r"].as<int>();
  leds[id].g = obj["g"].as<int>();
  leds[id].b = obj["b"].as<int>();
  leds[id].brightness = obj["brightness"].as<int>();

  updateStrip();

  JsonDocument response;
  JsonObject result = response.to<JsonObject>();

  addLEDState(result, id);

  sendJSON(200, response);
}

void handleOtherRoutes() {

  String path = server.uri();

  if (server.method() == HTTP_PUT && path.startsWith("/api/leds/")) {

    String idText = path.substring(String("/api/leds/").length());

    if (idText.length() != 1 || idText[0] < '0' || idText[0] > '9') {

      sendError(404, "Unknown LED ID");
      return;
    }

    int id = idText.toInt();

    if (id < 0 || id >= NUM_LEDS) {
      sendError(404, "Unknown LED ID");
      return;
    }

    handlePutLED(id);
    return;
  }

  sendError(404, "Resource not found");
}

void setup() {

  Serial.begin(115200);
  delay(1000);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  strip.begin();
  strip.clear();
  strip.show();

  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].r = 0;
    leds[i].g = 0;
    leds[i].b = 0;
    leds[i].brightness = 128;
  }

  WiFi.mode(WIFI_AP);

  bool started = WiFi.softAP(apSSID, apPassword, 1, 0, 2);

  if (!started) {
    Serial.println("AP startup failed!");
    return;
  }

  Serial.println("Access Point started");

  Serial.print("AP SSID: ");
  Serial.println(apSSID);

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  Serial.print("AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());

  server.on("/", HTTP_GET, handleRoot);

  server.on("/api/distance", HTTP_GET, handleDistance);

  server.on("/api/leds", HTTP_GET, handleGetLEDs);

  server.onNotFound(handleOtherRoutes);

  server.begin();

  Serial.println("HTTP server started");

  Serial.print("Open browser: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println("/");
}

void loop() {
  server.handleClient();
}