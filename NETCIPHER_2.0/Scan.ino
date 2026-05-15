void performDeauthMonitorSyncScan() {
    // Turn LED ON at start of scan
    digitalWrite(LED_PIN, HIGH);
    // Stop all attacks first
    apDeauthActive = false;
    stationDeauthActive = false;
    flood_active = false;
    deauthAllActive = false;
    beaconSpam.stop();
    if (isScanningStations) stopStationScan();
    if (hiddenRecoveryActive) stopHiddenRecovery();
    if (deauthMonitorActive) stopDeauthMonitor();
    
    // Stop AP and web server
    if (apAndWebRunning) {
        webServer.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apAndWebRunning = false;
        Serial.println("AP and Web server stopped for deauth monitor");
    }
    
    // Set WiFi to STA mode
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    // Draw scanning screen
    drawDeauthMonitorSyncScan();
    
    int n = WiFi.scanNetworks(false, true); // false = sync, true = include hidden
    Serial.printf("Scan complete. Networks found: %d\n", n);
    
    // Clear existing networks
    for (int i = 0; i < MAX_TARGETS; i++) {
        deauthNetworks[i] = _Network();
    }
    
    // Store scan results
    deauthNetworkCount = 0;
    if (n >= 0) {
        for (int i = 0; i < n && deauthNetworkCount < MAX_TARGETS; i++) {
            deauthNetworks[deauthNetworkCount].ssid = WiFi.SSID(i);
            memcpy(deauthNetworks[deauthNetworkCount].bssid, WiFi.BSSID(i), 6);
            deauthNetworks[deauthNetworkCount].ch = WiFi.channel(i);
            deauthNetworks[deauthNetworkCount].rssi = WiFi.RSSI(i);
            deauthNetworks[deauthNetworkCount].authmode = WiFi.encryptionType(i);
            deauthNetworks[deauthNetworkCount].selected = false;
            deauthNetworkCount++;
            
            // Debug output
            if (deauthNetworks[i].ssid.length() == 0) {
                Serial.printf("  %d: <HIDDEN> (BSSID: %s, CH: %d, RSSI: %d)\n", 
                    i, bytesToStr(deauthNetworks[i].bssid, 6).c_str(), 
                    deauthNetworks[i].ch, deauthNetworks[i].rssi);
            } else {
                Serial.printf("  %d: %s (BSSID: %s, CH: %d, RSSI: %d)\n", 
                    i, deauthNetworks[i].ssid.c_str(), 
                    bytesToStr(deauthNetworks[i].bssid, 6).c_str(), 
                    deauthNetworks[i].ch, deauthNetworks[i].rssi);
            }
        }
    } else {
        Serial.printf("Scan error: %d\n", n);
    }
    
    WiFi.scanDelete();
    
    // Show completion message briefly
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "SCAN COMPLETE");
    u8g2.drawLine(0, 15, 127, 15);
    u8g2.setFont(u8g2_font_8x13_tf);
    
    u8g2.sendBuffer();
    delay(1500);  // Show completion message for 1.5 seconds
    
    // Start the deauth monitor
    startDeauthMonitor();
    currentScreen = SCREEN_DEAUTH_MONITOR;
    // Turn LED OFF when scan is complete
    digitalWrite(LED_PIN, LOW);
}
void performScan() {
    // Turn LED ON at start of scan
    digitalWrite(LED_PIN, HIGH);
    Serial.println("Starting WiFi scan...");
    int n = WiFi.scanNetworks(false, true);
    Serial.print("Scan complete. Networks found: ");
    Serial.println(n);
    
    clearArray();
    
    if (n >= 0) {
        for (int i = 0; i < n && i < MAX_TARGETS; ++i) {
            _networks[i].ssid = WiFi.SSID(i);
            Serial.print("Network ");
            Serial.print(i);
            Serial.print(": ");
            Serial.println(_networks[i].ssid);
            
            memcpy(_networks[i].bssid, WiFi.BSSID(i), 6);
            _networks[i].ch = WiFi.channel(i);
            _networks[i].rssi = WiFi.RSSI(i);
            _networks[i].authmode = WiFi.encryptionType(i);
            _networks[i].selected = false;
        }
    } else {
        Serial.print("Scan error: ");
        Serial.println(n);
    }
    WiFi.scanDelete();   
    // Turn LED OFF when scan is complete
    digitalWrite(LED_PIN, LOW);
}
void startStationScan() {
    // STOP ALL ATTACKS FIRST
    if (!hasSelectedTargets()) {
        showNoTargetMessage();
        return;
    }
    stopAllAttacks();
    lastActivityTime = millis();
    digitalWrite(LED_PIN, HIGH);
    stationScanPacketCount = 0;
    

    if (apAndWebRunning) {
        webServer.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apAndWebRunning = false;
    }

    
    
    bool hasValidTarget = false;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i])) {
            hasValidTarget = true;
            break;
        }
    }
    
    if (!hasValidTarget) {
        showNoTargetMessage();
        return;
    }


    
    scannedStationCount = 0;
    currentTargetScanIndex = 0;
    stationScanCancelled = false;
    
    // Set WiFi to AP mode (enables raw frame transmission)
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_start();
    
    esp_wifi_set_promiscuous(true);
    
    // Capture ALL packet types for maximum station discovery
    wifi_promiscuous_filter_t filter = {
         .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | 
                        WIFI_PROMIS_FILTER_MASK_DATA | 
                        WIFI_PROMIS_FILTER_MASK_CTRL
    };
    esp_wifi_set_promiscuous_filter(&filter);
    
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallbackStation);
    
    // Select first AP
    if (!selectNextTargetForScan()) {
        // No selected AP found (should not happen)
        stopStationScan();
        return;
    }
    
    isScanningStations = true;
    stationScanStartTime = millis();
    apScanStartTime = millis();
    currentScreen = SCREEN_STATION_SCAN;
    
    Serial.printf("Starting scan with AP: %s on channel %d\n", 
                  currentScanSSID.c_str(), currentScanChannel);
}
void stopStationScan() {
    isScanningStations = false;
    stationScanCancelled = true;
    digitalWrite(LED_PIN, LOW);
    
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_stop();
    
    startAccessPoint();
    stationScanPacketCount = 0;
    
    Serial.printf("\n=== Scan Complete/Cancelled ===\nTotal stations found: %d\n", scannedStationCount);
    
    // Only change screen if we're not already on the station select screen
    if (currentScreen != SCREEN_STATION_SELECT) {
        currentScreen = SCREEN_STATION_SELECT;
        stationSelectIndex = 0;
        stationSelectStartIdx = 0;
    }
}

void startReactiveDeauth() {
    if (!hasSelectedTargets()) {
        showNoTargetMessage();
        return;
    }
    
    // Find the first selected AP
    bool found = false;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i])) {
            memcpy(deauth_target_bssid, _networks[i].bssid, 6);
            deauth_target_channel = _networks[i].ch;
            deauth_target_ssid = _networks[i].ssid;
            found = true;
            break;
        }
    }
    
    if (!found) {
        showNoTargetMessage();
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
    if (apDeauthActive) apDeauthActive = false;
    if (stationDeauthActive) stationDeauthActive = false;
    
    // Stop AP and web server if running
    if (apAndWebRunning) {
        webServer.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apAndWebRunning = false;
    }
WiFi.mode(WIFI_AP);


// Start WiFi interface
esp_wifi_start();

// Set channel to target AP's channel
esp_wifi_set_channel(deauth_target_channel, WIFI_SECOND_CHAN_NONE);

// Enable promiscuous mode for sniffing
esp_wifi_set_promiscuous(true);

wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT | 
                   WIFI_PROMIS_FILTER_MASK_DATA | 
                   WIFI_PROMIS_FILTER_MASK_CTRL
};
esp_wifi_set_promiscuous_filter(&filter);

// Set the callback
esp_wifi_set_promiscuous_rx_cb(promiscuousDeauthSniff);
    
    reactive_deauth_count = 0;
    auth_packet_count = 0;      
    reassoc_packet_count = 0;   
    sniff_deauth_active = true;
    reactiveDeauthActive = true;
    
    currentScreen = SCREEN_REACTIVE_DEAUTH;
    
}

void stopReactiveDeauth() {
    reactiveDeauthActive = false;
    resetDeauthIndices();
    sniff_deauth_active = false;
    
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    
    
    startAccessPoint();
    auth_packet_count = 0;
    reassoc_packet_count = 0;
    reactive_deauth_count = 0;
    
    Serial.println("Reactive deauth stopped");
}

void resetStationDeauthState() {
    staHopIndex = 0;
    lastStaHopTime = 0;
    lastStaDeauthSend = 0;
    staHopPackets = 0;
    stationOnlyIndex = 0;
    stationDeauthIndex = 0;
}
void rebuildStaHopChannels() {
    staHopCount = 0;
    bool chAdded[15] = {false};
    for (int i = 0; i < scannedStationCount; i++) {
        if (stationSelected[i] && stationAPChannel[i] >= 1 && stationAPChannel[i] <= 14) {
            int ch = stationAPChannel[i];
            if (!chAdded[ch]) {
                staHopChannels[staHopCount++] = ch;
                chAdded[ch] = true;
            }
        }
    }
    if (staHopCount == 0) {
        staHopChannels[0] = 1;
        staHopCount = 1;
    }
}
void resetDeauthIndices() {
    currentApDeauthIndex = 0;
    stationDeauthIndex = 0;
    stationOnlyIndex = 0;
    apFloodPattern = 0;
    stationFloodPattern = 0;
    resetApDeauthState();
    resetStationDeauthState();
}
void stopAllAttacks() {
    apDeauthActive = false;
    stationDeauthActive = false;
    combinedDeauthActive = false;
    flood_active = false;
    deauthAllActive = false;
    reactiveDeauthActive = false;
    attack_running = false;
    deauthing_active = false;
    
    if (beaconSpam.running) beaconSpam.stop();
    if (deauthMonitorActive) stopDeauthMonitor();
    if (hiddenRecoveryActive) stopHiddenRecovery();
    if (isScanningNearby) stopNearbyScan();
    
    resetDeauthIndices();
    packetsPerSecond = 0;
    digitalWrite(LED_PIN, LOW);
}