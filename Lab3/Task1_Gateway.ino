#include <WiFi.h>

const char* apSSID = "";
const char* apPassword = "";

const char* staSSID = "";
const char* staPassword = "";

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_AP_STA);

  bool apOK = WiFi.softAP(apSSID, apPassword, 1, 0, 2);

  Serial.print("AP started: ");
  Serial.println(apOK ? "YES" : "NO");

  Serial.print("Gateway AP IP: ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(staSSID, staPassword);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Upstream Wi-Fi connected");

    Serial.print("Gateway STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Upstream connection failed");
  }
}

void loop() {
  Serial.print("Sensor/client connections: ");
  Serial.println(WiFi.softAPgetStationNum());
  delay(3000);
}