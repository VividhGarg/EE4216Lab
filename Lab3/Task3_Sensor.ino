#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>

const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

const char* GATEWAY_URL = "http://192.168.4.1/api/sensor";

#define TRIG_PIN 5
#define ECHO_PIN 18

const unsigned long SEND_INTERVAL = 20000;
unsigned long lastSend = 0;

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

void sendDistance() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Gateway Wi-Fi disconnected");
    return;
  }

  float distance = measureDistance();

  if (!isfinite(distance) || distance < 2 || distance > 400) {
    Serial.println("Invalid distance reading");
    return;
  }

  JsonDocument doc;
  doc["node"] = "sensor01";
  doc["distance"] = distance;

  String payload;
  serializeJson(doc, payload);

  HTTPClient http;
  http.setTimeout(3000);

  if (!http.begin(GATEWAY_URL)) {
    Serial.println("HTTP initialization failed");
    return;
  }

  http.addHeader("Content-Type", "application/json");

  int status = http.POST(payload);

  Serial.print("Distance: ");
  Serial.print(distance, 2);
  Serial.print(" cm | Gateway HTTP: ");
  Serial.println(status);

  http.end();
}

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Sensor node started");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastRetry = 0;

    if (millis() - lastRetry >= 5000) {
      lastRetry = millis();
      WiFi.reconnect();
    }
  }

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();
    sendDistance();
  }
}