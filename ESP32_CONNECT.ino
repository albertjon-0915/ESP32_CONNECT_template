/*

  Firebase Arduino Client Library for ESP3266 and ESP32 by Mobizt --> needed library
  #include "addons/TokenHelper.h" --> addons for Firebase Client Library
  #include "addons/RTDBHelper.h" --> addons for Firebase Client Library
  #include <WiFi.h> --> already in custom library "connect.h"
  #include "time.h" --> built in library
  #define DISABLE_DEBUG --> comment this to do Serial.print/println
  #include <WebServer.h> --> already in custom library "routes.h"
  #include <Preferences.h> --> library to store persisted values
  #include <WebSocketsServer.h> --> https://github.com/Links2004/arduinoWebSockets, download and add manually
  700+ lines of code --> it aint ez haha
  
 */

// #define DISABLE_DEBUG
#include "connect.h"
#include "routes.h"
#include <DNSServer.h>
#include "utils.h"
#include "time.h"
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <Preferences.h>
#include <WebSocketsServer.h>



// const char* ssid = "";
// const char* password = "";
// const char* redir_ssid = "";
// const char* redir_password = "";
const char* API_KEY = "";  // Firebase API key
const char* DB_URL = "";   // Firebase Database URL
bool signUpOk = false;


// Captive portal essentials
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);                               // WiFiServer instance on port 80
WebSocketsServer webSocket = WebSocketsServer(81);  // start websocket at poprt 81 to prevent conflict with existing port 80
IPAddress apIP(192, 168, 4, 1);
bool shouldShutdownAP = false;
int count = 0;


const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;  // GMT+8 (Philippines time)
const int daylightOffset_sec = 0;     // No DST in PH


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;


CONNECT EspWiFi;
Params_wifi params;
Redirect_params redirect;
Preferences preferences;


String getLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  }

  char buf[16];
  strftime(buf, sizeof(buf), "%I:%M %p", &timeinfo);

  return String(buf);  // Convert char[] to Arduino String

  // eg. 3:45 PM
  // &timeinfo, "%H:%M:%S" --> 03:45:12
  // &timeinfo, "%d/%m/%Y   %Z" --> 09/11/25 UTC
  // Serial.println(&timeinfo, "%I:%M %p");
}

void fireBaseMutation() {
  if (Firebase.ready() && signUpOk) {
    /*
      Can use(set/get):
      - set
      - setInt
      - setFloat
      - setDouble
      - setString
      - setJSON
      - setArray
      - setBlob
      - setFile
    */
    if (Firebase.RTDB.setString(&fbdo, "Random/time", getLocalTime())) {
      Serial.println(getLocalTime());
      Serial.println("Successfully saved at" + fbdo.dataPath());
      Serial.println("( " + fbdo.dataPath() + " )");
    } else {
      Serial.println("Failed: " + fbdo.errorReason());
    }
  }
}

void fireBaseConnect() {
  // Firebase credentials
  config.api_key = API_KEY;
  config.database_url = DB_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Sign up, OK!");
    signUpOk = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void beginSTA() {
  String ssid = preferences.getString("redir_ssid", "");
  String password = preferences.getString("redir_password", "");
  params.ssid = ssid.c_str();
  params.password = password.c_str();

  if (ssid != "" && password != "") {
    manualRunSTA(params.ssid, params.password);

    if (WiFi.status() != WL_CONNECTED) {
      if (count == 40) { redirect.warn = true; }
      count != 40 && count++;
      Serial.print(".");
    }
  }
}

auto WrappedConfigTime = []() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
};


// wrap delays for better timing and handling
CREATE_ASYNC_FN(sendData, 5000, fireBaseMutation);  // params --> name, delay, function
CREATE_ASYNC_FN(staConnection, 1500, beginSTA);

// wrap functions to only run once
CREATE_RUNONCE_FN(connectFR, fireBaseConnect);
CREATE_RUNONCE_LAMBDA(runGetTime, WrappedConfigTime);  // directly wrap in the lambda, cannot store void on auto



void setup() {
  Serial.begin(115200);
  preferences.begin("store", false);

  // SET PARAMS HERE
  // params.ssid = ssid;
  // params.password = password;
  params.runAp = true;
  params.runSta = false;
  params.ap_ssid = "ESP_WIFI";  // change this line for hotspot name
  params.ap_password = "";      // change this line for hotspot password

  // WiFi connection
  EspWiFi.init(params);
  // addDomain --> to use different name on the URL instead of esp's IP
  // now instead of http://192.168.4.1, we use http://esportal32.local
  EspWiFi.addDomain("esportal32");

  // DNS -> redirect all domains to ESP
  dnsServer.start(DNS_PORT, "*", apIP);

  // Web server routes
  server.on("/", handleConnectWifiPortal);
  server.on("/disconnect", handleDisconnectPortal);
  server.on("/disconnect/success", handleDisconnect);
  server.begin();
}



void loop() {
  // getLocalTime();
  // fireBaseMutation();

  dnsServer.processNextRequest();
  server.handleClient();
  webSocket.loop();


  if (WiFi.status() != WL_CONNECTED) {
    redirect.isConnected = false;
    preferences.putBool("isConnected", false);
    asyncDelay(staConnection);
    return;
  }
  
  preferences.putBool("isConnected", true);
  redirect.isConnected = true;

  connectFR.run();   // run Firebase connection once --> auth
  runGetTime.run();  // Init and get the time

  asyncDelay(sendData);
}
