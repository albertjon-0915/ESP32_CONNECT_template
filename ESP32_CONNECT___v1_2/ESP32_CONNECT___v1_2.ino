/*

*/
// #define DISABLE_DEBUG
#include <Preferences.h>
#include <DNSServer.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "connect.h"
#include "utils.h"

// CONSTANT DECLARATIONS
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);
IPAddress apIP(192, 168, 4, 1);

// FIREBASE CONSTANTS/DECLARATIONS
const char* API_KEY = "";  // Firebase API key
const char* DB_URL = "";   // Firebase Database URL
bool signUpOk = false;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ESP32 PARAMETERS
params.runAp = true;
params.runSta = false;
params.ap_ssid = "esp32";  // change this line for AP ssid
params.ap_password = "";   // change this line for AP password

// CLASS DECLARATIONS
CONNECT esp32;

// FUNCTION WRAPPER --> delays
CREATE_ASYNC_FN(sendData, 5000, fireBaseMutation);

// FUNCTION WRAPPER --> run once
CREATE_RUNONCE_FN(connectFR, fireBaseConnect);


// SETUP LOGIC
void setup() {
  Serial.begin(115200);
  preferences.begin("store", false);
  esp32.init(params);
  EspWiFi.addDomain("esp32"); // add domain to AP --> http://esp32.local
  dnsServer.start(DNS_PORT, "*", apIP); // DNS -> redirect all domains to ESP


  // WEB SERVER ROUTES
  server.on("/", handleConnectWifiPortal);
  server.on("/disconnect", handleDisconnectPortal);
  server.on("/disconnect/success", handleDisconnect);
  server.begin();
}

// LOOP LOGIC
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}