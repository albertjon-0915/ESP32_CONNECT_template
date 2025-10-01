/*

*/

void serverHandlers() {
  MDNS.update();
  dnsServer.processNextRequest();
  server.handleClient();
}

void serverRoutes() {
  server.on("/", handleConnectWifiPortal);
  server.on("/disconnect", handleDisconnectPortal);
  server.on("/disconnect/success", handleDisconnect);
}

void fireBaseMutation() {
  if (Firebase.ready() && signUpOk) {
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
