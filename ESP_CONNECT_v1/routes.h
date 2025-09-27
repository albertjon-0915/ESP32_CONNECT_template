#ifndef ROUTES_H
#define ROUTES_H

#include <WiFi.h>
#include <Preferences.h>
#include <WebServer.h>
#include <pgmspace.h>
#include "connect.h"

struct Redirect_params {
  String redir_ssid;
  String redir_password;
  bool isConnected = false;
  bool warn = false;
};

extern WebServer server;
extern Redirect_params redirect;
extern Preferences preferences;
extern bool shouldShutdownAP;
extern int count;


// Disconnect Portal
inline void handleDisconnectPortal() {
  redirect.redir_ssid = preferences.getString("redir_ssid", "");
  redirect.redir_password = preferences.getString("redir_password", "");


  static const char pageHeader[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Reset Wi-Fi Connection</title>
        <style>
            body { font-family: -apple-system, BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;
                   background-color:#1a1a1a;color:#e0e0e0;display:flex;align-items:center;justify-content:center;
                   min-height:100vh;padding:1rem;margin:0; }
            .portal-container { position:relative;background-color:#242424;padding:2.5rem;border-radius:1.5rem;width:100%;max-width:24rem;
                                display:flex;flex-direction:column;align-items:center;gap:0.75rem;border:1px solid #333; }
            .back-link{position:absolute;top:1.5rem;left:1.5rem;color:#e0e0e0;text-decoration:none;transition:.2s ease;display:flex;align-items:center;gap:.5rem}
            .back-link:hover{color:#999;transform:scale(1.05);text-shadow:0 0 5px rgba(255,255,255,.2)}
            .portal-heading { font-size:1.875rem;font-weight:500;text-align:center; }
            .status-indicator { background: transparent; color: #3cb371; padding: 0.75rem 0; width: 100%; font: 500 0.875rem/1 sans-serif; text-align: center; margin-top: 0; border: none; }
            .network-info { display: flex; flex-direction: column; align-items: center; width: 100%; font: 0.875rem/1 sans-serif; color: #b0b0b0; padding: 0.5rem; background: #2a2a2a; border: 1px solid #333; border-radius: 0.5rem; margin-bottom: 0.75rem; }
            .network-info > div { display: flex; justify-content: space-between; width: 100%; padding: 0.25rem 0; }
            .network-label { color: #999; font-weight: 400; }
            .network-value { color: #e0e0e0; font-weight: 500; }
            #reset-tag { font-size:0.75rem;color:#ef4444;text-decoration:none;display:flex;align-items:center;gap:0.5rem;
                         padding:0.2rem 2rem 0.2rem 1.2rem;border:1px solid #444;border-radius:9999px;
                         transition:background-color 0.2s ease,color 0.2s ease,border-color 0.2s ease;margin-top:1.5rem; }
            #reset-tag span{ font-size: clamp(0.6rem, 2vw, 1rem); }
            #reset-tag:hover { background-color:rgba(239,68,68,0.1); color:#fca5a5; border-color:#ef4444; }
        </style>
    </head>
    <body>
        <div class="portal-container">
            <a href="http://192.168.4.1" class="back-link">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M15 18l-6-6 6-6"/></svg>
            <span>back</span></a>
            <h1 class="portal-heading">Device Status</h1>
            <p class="status-indicator">Your device has saved credentials</p>
    )rawliteral";

  char buffer[512];
  snprintf(buffer, sizeof(buffer),
           R"rawliteral(
        <div class="network-info">
            <div><span class="network-label">SSID:</span><span class="network-value">%s</span></div>
            <div><span class="network-label">Password:</span><span class="network-value">%s</span></div>
        </div>
        )rawliteral",
           redirect.redir_ssid.c_str(),
           redirect.redir_password.c_str());

  static const char pageFooter[] PROGMEM = R"rawliteral(
        <a href="disconnect/success" id="reset-tag">
            <svg class="w-3 h-1" viewBox="0 0 20 20" fill="currentColor" style="color: #ef4444;">
                <circle cx="10" cy="10" r="7" />
            </svg>
            <span class="font-medium">Reset WiFi Connection</span>
        </a>
        </div>
    </body>
    </html>
    )rawliteral";

  server.sendContent_P(pageHeader);
  server.sendContent(buffer);
  server.sendContent_P(pageFooter);
}


// Handle Disconnection
inline void handleDisconnect() {
  preferences.remove("redir_ssid");
  preferences.remove("redir_password");
  preferences.putBool("isConnected", false);
  redirect.warn = false;
  count = 0;

  refreshSTA();

  static const char page[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <body>
        <h3>Disconnection Successful, Deleted saved credentials</h3>
        <h5>Redirecting... please wait to connect to other Wi-Fi</h5>
        <script>
            document.addEventListener("DOMContentLoaded", () => {
                setTimeout(() => { window.location.href = "/"; }, 5000);
            });
        </script>
    </body>
    </html>
  )rawliteral";

  server.send_P(200, "text/html", page);
}

// Wi-Fi Connect Portal
inline void handleConnectWifiPortal() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    redirect.redir_ssid = server.arg("ssid");
    redirect.redir_password = server.arg("password");

    preferences.putString("redir_ssid", redirect.redir_ssid);
    preferences.putString("redir_password", redirect.redir_password);

    WiFi.begin(redirect.redir_ssid, redirect.redir_password);
  }

  if (preferences.isKey("isConnected")) {
    redirect.isConnected = preferences.getBool("isConnected", false);
  }

  redirect.redir_ssid = preferences.getString("redir_ssid", "");
  redirect.redir_password = preferences.getString("redir_password", "");
  String isHidden = redirect.redir_ssid != "" && redirect.redir_password != "" ? "" : "hidden";
  String isConnecting = redirect.isConnected ? "Connected" : "Connecting... please wait";
  String isHideNetwork = isConnecting == "Connected" ? "" : "hidden";
  String isWarn = !redirect.isConnected && redirect.warn && (redirect.redir_ssid != "" && redirect.redir_password != "") ? "" : "hidden";
  String isDisabled = redirect.isConnected || (redirect.redir_ssid != "" && redirect.redir_password != "") ? "disabled" : "";


  static const char page_part1[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wi-Fi Connect Portal</title>
    <style>body{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,"Helvetica Neue",Arial,sans-serif;
    background-color:#1a1a1a;color:#e0e0e0;display:flex;align-items:center;justify-content:center;
    min-height:100vh;padding:1rem;margin:0;}.portal-container{background-color:#242424;padding:2.5rem;border-radius:1.5rem;
    box-shadow:0 10px 15px -3px rgba(0,0,0,0.2),0 4px 6px -2px rgba(0,0,0,0.1);width:100%;max-width:24rem;
    display:flex;flex-direction:column;align-items:center;gap:1.5rem;border:1px solid #333;}
    .portal-heading{font-size:1.875rem;font-weight:500;text-align:center;letter-spacing:0.05em;}
    .input-field{background-color:#2a2a2a;color:#f0f0f0;border:1px solid #444;width:100%;padding:0.75rem;border-radius:0.75rem;}
    .input-field:focus{outline:none;border-color:#555;box-shadow:0 0 0 2px rgba(255,255,255,0.1);}
    .connect-button{background-color:#3a3a3a;border:1px solid #444;color:#f0f0f0;width:65%;padding:0.75rem;
    font-weight:600;border-radius:0.75rem;font-size:1.125rem;margin-top:0.5rem;cursor:pointer;}
    .connect-button:hover:not(:disabled){background-color:#444;transform:translateY(-2px);}
    .connect-button:disabled{background-color:#2a2a2a;border:1px solid #444;color:#999;opacity:.6;cursor:not-allowed;pointer-events:none;}
    .message-box{font-size:0.875rem;font-weight:300;color:#f87171;margin-top:0.5rem;display:none;}
    .status-indicator{background-color:transparent;padding:0.75rem 0;border:none;text-align:center;width:100%;
    font-size:0.875rem;font-weight:500;}
    .green{color:#3cb371;}.yellow{color:#ffcb5b;}.font-md{font-size:1.2rem;font-weight:600;}.font-sm{font-size:1rem;font-weight:400;}
    .status-connected{display:inline-flex;align-items:center;margin-right:1rem;}
  )rawliteral";

  static const char page_part2[] PROGMEM = R"rawliteral(
    .status-network{font-size:0.9rem;font-weight:400;color:#44b76e;}.circled-indicator{font-size:0.7rem;margin-right:8px;}
    #reset-tag{font-size:0.75rem;color:#aaaaaa;text-decoration:underline;cursor:pointer;transition:color 0.2s ease;}
    #reset-tag:hover{color:#dddddd;}
    .is-hidden{display:none;}</style></head><body>
    <div class="portal-container">
    <h1 class="portal-heading">Network Portal</h1>
    <div class="status-indicator green" %s>
      <div class="status-connected font-md green"><span class="circled-indicator">🟢 </span>%s</div>
      <div class="status-network green" %s>%s</div>
    </div>
    <div id="connection-status" class="status-indicator yellow" %s>
      <div class="status-connected dont-sm yellow">Still waiting?... Let's try a reset and reconnect</div>
    </div>
    <input id="ssid-input" type="text" placeholder="Network Name (SSID)" class="input-field" %s>
    <input id="password-input" type="password" placeholder="Password" class="input-field" %s>
    <button id="connect-button" class="connect-button" %s>Connect</button>
    <div id="message-box" class="message-box"></div>
    <a href="/disconnect" id="reset-tag" %s>Disconnect and reset wifi connection?</a>
    </div>
  )rawliteral";

  static const char page_part3[] PROGMEM = R"rawLiteral(
    <script>
      const connectButton=document.getElementById('connect-button');
      const ssidInput=document.getElementById('ssid-input');
      const passwordInput=document.getElementById('password-input');
      const messageBox=document.getElementById('message-box');
      connectButton.addEventListener('click',()=>{
        const ssid=ssidInput.value.trim();
        const password=passwordInput.value.trim();
        if(!ssid){showMessage('Please enter the network SSID.');return;}
        setTimeout(() => { window.location.href = "/"; }, 60000);
        const url=new URL(window.location.href);
        url.searchParams.set('ssid',ssid);
        url.searchParams.set('password',password);
        fetch(url.toString()).then(res=>res.text()).then(html=>{document.body.innerHTML=html;});
      });
      function showMessage(message){messageBox.textContent=message;messageBox.style.display='block';
      setTimeout(()=>{messageBox.style.display='none';},3000);}
    </script></body></html>
  )rawLiteral";

  char buffer[4096];
  snprintf_P(buffer, sizeof(buffer), page_part2,
             isHidden.c_str(),             // hide
             isConnecting.c_str(),         // label
             isHideNetwork.c_str(),        // hidden
             redirect.redir_ssid.c_str(),  // network name
             isWarn.c_str(),
             isDisabled.c_str(),
             isDisabled.c_str(),
             isDisabled.c_str(),
             isHidden.c_str());

  server.sendContent_P(page_part1);
  server.sendContent(buffer);
  server.sendContent_P(page_part3);
}

#endif  // ROUTES_H
