void promiscuousCallbackNearby(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!isScanningNearby || nearbyScanCancelled) return;
    if (type != WIFI_PKT_DATA) return;

    const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* payload = pkt->payload;
    int8_t rssi = pkt->rx_ctrl.rssi;
    if (pkt->rx_ctrl.sig_len < 24) return;

    uint8_t frameControl1 = payload[1];
    bool toDS = (frameControl1 & 0x01);
    bool fromDS = (frameControl1 & 0x02) >> 1;

    uint8_t addr1[6], addr2[6], addr3[6];
    memcpy(addr1, &payload[4], 6);
    memcpy(addr2, &payload[10], 6);
    memcpy(addr3, &payload[16], 6);

    uint8_t* stationMac = NULL;
    uint8_t* apBSSID = NULL;

    if (!fromDS && toDS) {
        stationMac = addr2;
        apBSSID = addr1;
    } else if (fromDS && !toDS) {
        stationMac = addr1;
        apBSSID = addr2;
    } else {
        return;
    }
    if (stationMac == NULL || apBSSID == NULL) return;

    // Skip broadcast/multicast
    if (stationMac[0] == 0xFF && stationMac[1] == 0xFF && stationMac[2] == 0xFF &&
        stationMac[3] == 0xFF && stationMac[4] == 0xFF && stationMac[5] == 0xFF)
        return;
    if (stationMac[0] == 0x01 && stationMac[1] == 0x00 && stationMac[2] == 0x5E) return;
    if (stationMac[0] == 0x33 && stationMac[1] == 0x33) return;

    // Skip if station MAC matches any known AP BSSID
    bool isAP = false;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].ch > 0 && memcmp(_networks[i].bssid, stationMac, 6) == 0) {
            isAP = true;
            break;
        }
    }
    if (isAP) return;

    // Find AP SSID from known networks
    String apSSID = "";
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].ch > 0 && memcmp(_networks[i].bssid, apBSSID, 6) == 0) {
            apSSID = _networks[i].ssid;
            if (apSSID.length() == 0) apSSID = "<HIDDEN>";
            break;
        }
    }
    if (apSSID.length() == 0) {
        char bssidStr[18];
        snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 apBSSID[0], apBSSID[1], apBSSID[2], apBSSID[3], apBSSID[4], apBSSID[5]);
        apSSID = "[" + String(bssidStr) + "]";
    }

    // Check if station already exists (raw compare)
    bool found = false;
    for (int i = 0; i < scannedStationCount; i++) {
        if (memcmp(scannedStationMAC[i], stationMac, 6) == 0) {
            found = true;
            if (stationAPChannel[i] == 0) {
                memcpy(stationAPBSSIDRaw[i], apBSSID, 6);
                stationAPChannel[i] = nearbyCurrentChannel;
                stationNetworkSSID[i] = apSSID;
            }
            break;
        }
    }

    if (!found && scannedStationCount < MAX_STATIONS) {
        memcpy(scannedStationMAC[scannedStationCount], stationMac, 6);
        memcpy(stationAPBSSIDRaw[scannedStationCount], apBSSID, 6);
        stationAPChannel[scannedStationCount] = nearbyCurrentChannel;
        stationNetworkSSID[scannedStationCount] = apSSID;
        stationSelected[scannedStationCount] = false;
        scannedStationCount++;

        char macStr[18], bssidStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 stationMac[0], stationMac[1], stationMac[2],
                 stationMac[3], stationMac[4], stationMac[5]);
        snprintf(bssidStr, sizeof(bssidStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 apBSSID[0], apBSSID[1], apBSSID[2], apBSSID[3], apBSSID[4], apBSSID[5]);
        Serial.printf("[NEARBY] Station: %s -> AP: %s (BSSID: %s, CH: %d, RSSI: %d)\n",
                      macStr, apSSID.c_str(), bssidStr, nearbyCurrentChannel, rssi);
    }
}

void startNearbyScan() {
    stopAllAttacks();

    if (apAndWebRunning) {
        webServer.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apAndWebRunning = false;
    }

    // Clear all station data (raw arrays)
    scannedStationCount = 0;
    for (int i = 0; i < MAX_STATIONS; i++) {
        memset(scannedStationMAC[i], 0, 6);
        memset(stationAPBSSIDRaw[i], 0, 6);
        stationAPChannel[i] = 0;
        stationNetworkSSID[i] = "";
        stationSelected[i] = false;
    }

    nearbyScanCancelled = false;
    nearbyCurrentChannel = 1;

    WiFi.mode(WIFI_OFF);
    delay(100);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_AP);
    esp_wifi_start();

    esp_wifi_set_promiscuous(true);
    wifi_promiscuous_filter_t filter = { .filter_mask = WIFI_PROMIS_FILTER_MASK_DATA };
    esp_wifi_set_promiscuous_filter(&filter);
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallbackNearby);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    isScanningNearby = true;
    nearbyScanStartTime = millis();
    currentScreen = SCREEN_NEARBY_SCAN;

    Serial.println("Nearby scan started - scanning all channels for stations");
}

void stopNearbyScan() {
    isScanningNearby = false;
    nearbyScanCancelled = true;

    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
    esp_wifi_stop();

    startAccessPoint();

    Serial.printf("Nearby scan complete. Total stations found: %d\n", scannedStationCount);
    currentScreen = SCREEN_NEARBY_RESULTS;
    nearbyScrollOffset = 0;
    nearbySelectedIndex = 0;
}

void drawNearbyScanScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "SCANNING");
    u8g2.drawLine(0, 15, 127, 15);

    static unsigned long lastChan = 0;
    if (millis() - lastChan > 150) {
        nearbyCurrentChannel = (nearbyCurrentChannel % 14) + 1;
        esp_wifi_set_channel(nearbyCurrentChannel, WIFI_SECOND_CHAN_NONE);
        lastChan = millis();
    }

    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(0, 35, ("STA: " + String(scannedStationCount)).c_str());
    u8g2.drawStr(0, 50, ("CH: " + String(nearbyCurrentChannel)).c_str());

    int elapsed = (millis() - nearbyScanStartTime) / 1000;
    int progress = (elapsed * 100) / 60;
    if (progress > 100) progress = 100;

    u8g2.drawFrame(0, 58, 128, 5);
    u8g2.drawBox(1, 59, (progress * 126) / 100, 3);
    u8g2.setCursor(60, 57);
    u8g2.print(String(elapsed) + "s");

    u8g2.sendBuffer();
}

void drawNearbyResultsScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "NEARBY DEVICES");
    u8g2.drawLine(0, 15, 127, 15);
    u8g2.setFont(u8g2_font_6x10_tf);

    if (scannedStationCount == 0) {
        u8g2.setFont(u8g2_font_8x13_tf);
        u8g2.drawStr(20, 35, "NO DEVICES");
    } else {
        for (int i = 0; i < 4 && (i + nearbyScrollOffset) < scannedStationCount; i++) {
            int idx = i + nearbyScrollOffset;
            int y = 27 + (i * 11);

            if (idx == nearbySelectedIndex) {
                u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT-3, verticalBar);
            }

            if (stationSelected[idx]) {
                u8g2.drawXBM(7, y - 8, 8, 8, checkmark);
            } else {
                u8g2.drawFrame(7, y - 8, 8, 8);
            }

            // Convert raw MAC to string for display
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     scannedStationMAC[idx][0], scannedStationMAC[idx][1],
                     scannedStationMAC[idx][2], scannedStationMAC[idx][3],
                     scannedStationMAC[idx][4], scannedStationMAC[idx][5]);
            u8g2.drawStr(20, y, String(macStr).substring(0, 12).c_str());
        }
    }
    u8g2.sendBuffer();
}