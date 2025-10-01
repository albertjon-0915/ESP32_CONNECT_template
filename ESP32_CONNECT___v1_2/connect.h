#ifndef CONNECT_H
#define CONNECT_H

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DNSServer.h>

DNSServer dnsServer;
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);

struct Params_wifi {
  const char* ssid;
  const char* password;
  wifi_mode_t mode;
  const char* ap_ssid;
  const char* ap_password;
  bool runAp = false;
  bool runSta = true;
  bool startServer = false;

  void (*webServers)() = nullptr;
};

enum Dns_Target {
  DEFAULT_DNS,
  CHANGE_DNS
};

// extern WiFiServer server; --> WebServer is running on top of this
extern WebServer server;
extern IPAddress apIP;

class CONNECT {
public:
  void init(
    const char* ssid,
    const char* password,
    wifi_mode_t mode = WIFI_AP_STA,
    const char* ap_ssid = "",
    const char* ap_password = "",
    bool runAp = false,
    bool runSta = true,
    bool startServer = false,
     void (*onConnect)() = nullptr);
  void init(const Params_wifi& params);

  void addDomain(const char* domain);
  void changeDNS(const char* domain);
  String getIpAddress();
  void addServer(void (*setup)(WiFiClient&));

private:
  const char* ssid;
  const char* password;
  const char* ap_ssid;
  const char* ap_password;
  const char* domain;
  bool isServer = false;
  bool runAp = false;
  bool runSta = true;
  void (*onConnect)() = nullptr; 
};

inline void CONNECT::init(const Params_wifi& params) {
  init(
    params.ssid,
    params.password,
    params.mode,
    params.ap_ssid,
    params.ap_password,
    params.runAp,
    params.runSta,
    params.startServer,
    params.webServers);
}

inline void CONNECT::init(
  const char* ssid, const char* password, wifi_mode_t mode, const char* ap_ssid, const char* ap_password, bool runAp, bool runSta, bool startServer, void (*webServers)()) {

  this->ssid = ssid;
  this->password = password;
  this->ap_ssid = ap_ssid;
  this->ap_password = ap_password;
  this->isServer = startServer;
  this->runAp = runAp;
  this->runSta = runSta;

  WiFi.mode(mode);

  if (runAp) {
    // IPAddress apIP(192, 168, 4, 1);  // ESP32 AP IP
    WiFi.softAP(ap_ssid, ap_password);  // start AP mode
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    Serial.print("AP IP address: ");  // This is for hotspot IP
    Serial.println(WiFi.softAPIP());
  }

  if (runSta) {
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("\nWiFi connected");
  }

  if (isServer) {
    if (this->webServers) {
      this->webServers();   // call callback
    }

    server.begin();
    Serial.println("Server started");
  }
}

inline void CONNECT::addDomain(const char* domain) {
  this->domain = domain;
  if (MDNS.begin(domain)) {
    Serial.print("MDNS responder started at http://");
    Serial.print(domain);
    Serial.println(".local");
  } else {
    Serial.println("Error starting MDNS");
  }
}

inline void CONNECT::changeDNS(const char* domain, Dns_Target target) {
  const char* prevDomain = this->domain;

  MDNS.end();
  switch (target) {
    case DEFAULT_DNS:
      MDNS.begin(defaultIP);
      break;
    case CHANGE_DNS:
      MDNS.begin(domain);
      break;
    default:
      MDNS.begin(prevDomain);
      break;
  }
}

inline void manualRunSTA(const char* sta_ssid, const char* sta_pass) {
  WiFi.begin(sta_ssid, sta_pass);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
  }
}

inline void refreshSTA() {
  if (WiFi.getMode() & WIFI_MODE_STA) {
    WiFi.disconnect(true);
    Serial.println("STA disconnected");
  }
}

// NOTE: Use this if we are using WifiServer not WebServer
// inline void CONNECT::addServer(void (*setup)(WiFiClient&)) {
//   WiFiClient client = server.available();
//   if (client) {
//     Serial.println("New Client Connected");

//     setup(client);

//     client.stop();
//     Serial.println("Client Disconnected");
//   }
// }

// inline String CONNECT::getIpAddress() {
//   return WiFi.localIP().toString();
// }

#endif  // CONNECT_H