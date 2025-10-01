#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  // optional: can be AP, STA or both
  WiFi.mode(WIFI_MODE_APSTA);
  Serial.println("Waiting for SmartConfig...");

  WiFi.beginSmartConfig(); // Start SmartConfig

  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nSmartConfig received.");
  Serial.print("Connecting to WiFi: ");
  Serial.println(WiFi.SSID());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Your code here
}
