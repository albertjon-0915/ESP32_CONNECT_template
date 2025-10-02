#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

Preferences prefs;
WebServer server(80);

const char* ap_ssid = "espGo";
const char* ap_pass = "";

void startSmartConfig() {
  Serial.println("Starting SmartConfig...");
  WiFi.beginSmartConfig();

  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nSmartConfig received.");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("STA IP: ");
  Serial.println(WiFi.localIP());

  // Save new credentials
  prefs.begin("wifi", false);
  prefs.putString("ssid", WiFi.SSID());
  prefs.putString("pass", WiFi.psk());
  prefs.end();
  Serial.println("WiFi credentials saved.");

  WiFi.stopSmartConfig(); // stop listening until requested again
}

void handleRoot() {
  server.send(200, "text/html",
    "<h1>ESP32 WiFi Config</h1>"
    "<p><a href=\"/reset\"><button>Reset WiFi & Start SmartConfig</button></a></p>");
}

void handleReset() {
  prefs.begin("wifi", false);
  prefs.clear();  // delete saved SSID/pass
  prefs.end();

  server.send(200, "text/html", "<p>Credentials cleared. Restarting SmartConfig...</p>");
  delay(1000);
  startSmartConfig();
}

void setup() {
  Serial.begin(115200);

  // AP + STA mode
  WiFi.mode(WIFI_MODE_APSTA);

  // Start AP so user can always connect
  WiFi.softAP(ap_ssid, ap_pass);
  Serial.print("AP started, connect to SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());   // usually 192.168.4.1

  // Load saved WiFi creds
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid != "") {
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.printf("Connecting to saved WiFi: %s\n", ssid.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print(".");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to STA WiFi!");
    Serial.print("STA IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nNo STA WiFi or failed, waiting for reset via AP...");
  }

  // Setup web server (accessible via AP IP)
  server.on("/", handleRoot);
  server.on("/reset", handleReset);
  server.begin();
  Serial.println("Web server started! Access via AP at http://192.168.4.1/");
}

void loop() {
  server.handleClient();
}
