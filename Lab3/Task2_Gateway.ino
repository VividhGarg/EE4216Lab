#include <WiFi.h>

const char* ssid = "";
const char* password = "";

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Gateway AP");

    Serial.print("Sensor Node IP: ");
    Serial.println(WiFi.localIP());

    Serial.print("Gateway IP: ");
    Serial.println(WiFi.gatewayIP());
  } else {
    Serial.println("Connection failed");
  }
}

void loop() {
  delay(1000);
}
