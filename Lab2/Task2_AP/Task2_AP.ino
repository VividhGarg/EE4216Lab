#include <WiFi.h>

const char* apSSID = "A0284640A";
const char* apPassword = "HelloWorld";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nEE4216 Lab 2 - Task 2");

  WiFi.mode(WIFI_AP);

  bool success = WiFi.softAP(apSSID, apPassword, 1, 0, 2);

  if (success) {
    Serial.println("Access Point started!");

    Serial.print("AP SSID: ");
    Serial.println(apSSID);

    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("AP MAC Address: ");
    Serial.println(WiFi.softAPmacAddress());

    Serial.println("Maximum clients: 2");

  } else {
    Serial.println("Failed to start Access Point.");
  }
}

void loop() {
  Serial.print("Connected stations: ");
  Serial.println(WiFi.softAPgetStationNum());

  delay(3000);
}
