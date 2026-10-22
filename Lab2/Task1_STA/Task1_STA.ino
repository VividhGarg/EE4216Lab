#include <WiFi.h>

const char* ssid = "TP-Link_6FCB";
const char* password = "72972786";

const unsigned long TIMEOUT_MS = 15000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\nEE4216 Lab 2 - Task 1");
  Serial.println("Connecting in STA mode...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startTime < TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi Connected!");

    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    Serial.print("STA IP Address: ");
    Serial.println(WiFi.localIP());

    Serial.print("STA MAC Address: ");
    Serial.println(WiFi.macAddress());

  } else {
    Serial.println("Wi-Fi connection failed.");
    Serial.println("Check SSID, password and 2.4 GHz support.");

    Serial.print("Connection status: ");
    Serial.println(WiFi.status());
  }
}

void loop() {
}
