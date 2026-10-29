#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "";
const char* password = "";

#define TRIG_PIN 5
#define ECHO_PIN 18

const char* gatewayURL = "http://192.168.4.1/api/sensor";

unsigned long lastSend = 0;

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

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Sensor node starting");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    delay(200);
    return;
  }

  if (millis() - lastSend >= 5000) {
    lastSend = millis();

    float distance = measureDistance();

    if (distance < 2 || distance > 400 || !isfinite(distance)) {
      Serial.println("Invalid distance");
      return;
    }

    HTTPClient http;
    http.setTimeout(3000);
    http.begin(gatewayURL);
    http.addHeader("Content-Type", "application/json");

    String body = "{\"node\":\"sensor01\",\"distance\":" + String(distance, 2) + "}";

    int status = http.POST(body);

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" cm | HTTP status: ");
    Serial.println(status);

    http.end();
  }
}