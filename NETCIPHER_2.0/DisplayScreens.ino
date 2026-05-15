void drawHiddenRecoverScreen() {

    // Turn off LED after blink
  if (pulseState && (millis() - lastPulseUpdate > 50)) {
      digitalWrite(LED_PIN, LOW);
      pulseState = false;
  }
  u8g2.clearBuffer();

  // Heading
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.drawStr(5, 12, "RECOVERING...");
  u8g2.drawLine(0, 15, 127, 15);

  // AP MAC (target BSSID) in brackets
  u8g2.setFont(u8g2_font_6x10_tf);
  String bssidStr = "[" + bytesToStr(hiddenTargetBSSID, 6) + "]";
  u8g2.drawStr(0, 32, bssidStr.c_str());

  // Station MAC (source) in brackets
  u8g2.setFont(u8g2_font_6x10_tf);
  
  u8g2.drawStr(0, 50, "CAPTURING...");
  

  u8g2.sendBuffer();
}
void drawHiddenSuccessScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_8x13_tf);
  u8g2.drawStr(5, 12, "SSID RECOVERED");
  u8g2.drawLine(0, 15, 127, 15);
  
  u8g2.setFont(u8g2_font_8x13_tf);
  
  // SCROLLING TEXT FOR LONG SSID
  if (hiddenRecoveredSSID.length() > 16) {
    // Update scrolling position
    if (millis() - lastScrollUpdate > 300) {
      scrollPos = (scrollPos + 1) % (hiddenRecoveredSSID.length() + 2);
      lastScrollUpdate = millis();
    }
    
    // Create scrolling window
    String scrollText = hiddenRecoveredSSID + "  "; // Add spaces for pause
    int pos = scrollPos;
    if (pos > scrollText.length() - 16) {
      pos = 0; // Reset to beginning
    }
    String toShow = scrollText.substring(pos, pos + 16);
    u8g2.drawStr(0, 35, toShow.c_str());
  } else {
    // SSID fits, show normally
    u8g2.drawStr(0, 35, hiddenRecoveredSSID.c_str());
  }
  
  
  u8g2.sendBuffer();
}
void drawDeauthAllScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "DEAUTH ALL");
    u8g2.drawLine(0, 15, 127, 15);
    
    if (deauthAllNetworkCount == 0) {
        u8g2.setFont(u8g2_font_8x13_tf);
        u8g2.drawStr(10, 35, "No networks");
        
    } else {
        // Show current target
        _Network& cur = deauthAllNetworks[deauthAllCurrentIndex];
        String ssid = cur.ssid.length() > 0 ? cur.ssid : "<HIDDEN>";
        
        u8g2.setFont(u8g2_font_6x10_tf);
        
        // SSID only (no "AP:" label)
        u8g2.drawStr(0, 32, ssid.c_str());
        
        // Channel
        u8g2.drawStr(0, 47, "CH:");
        u8g2.setCursor(25, 47);
        u8g2.print(cur.ch);
        
        // Progress indicator
        u8g2.drawStr(0, 62, (String(deauthAllCurrentIndex+1) + "/" + String(deauthAllNetworkCount)).c_str());
    }
    
    u8g2.sendBuffer();
}
void drawDeauthMonitorSyncScan() {
    static int dotCount = 0;
    static unsigned long lastDotUpdate = 0;
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "SCANNING");
    u8g2.drawLine(0, 15, 127, 15);
    u8g2.sendBuffer();
}
void drawDeauthScreen() {
    u8g2.clearBuffer();
    
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "DEAUTHER");
    u8g2.drawLine(0, 15, 127, 15);
    u8g2.setFont(u8g2_font_8x13_tf);
    
    int apCount = getSelectedTargetCount();
    int staCount = getSelectedStationCount();
    
    // Check if deauth is actively running
    bool isDeauthRunning = (attack_running || stationDeauthActive || apDeauthActive);
    
    // Show what's selected
    if (apCount > 0 && staCount > 0) {
        // Both selected
        u8g2.drawStr(0, 30, "APS:");
        u8g2.drawStr(40, 30, String(apCount).c_str());
        u8g2.drawStr(0, 45, "STAS:");
        u8g2.drawStr(40, 45, String(staCount).c_str());
        
        // ONLY show packet rate line if deauth is running
        if (isDeauthRunning) {
            u8g2.drawStr(80, 30, (String(packetsPerSecond) + "p/s").c_str());
        }
        // When not running, draw NOTHING there (line is hidden)
    } 
    else if (apCount > 0) {
        // Only APs selected
        u8g2.drawStr(0, 30, "APS:");
        u8g2.drawStr(40, 30, String(apCount).c_str());
        
        if (isDeauthRunning) {
            u8g2.drawStr(80, 30, (String(packetsPerSecond) + "p/s").c_str());
        }
    } 
    else if (staCount > 0) {
        // Only stations selected
        u8g2.drawStr(0, 30, "STAS:");
        u8g2.drawStr(40, 30, String(staCount).c_str());
        
        if (isDeauthRunning) {
            u8g2.drawStr(80, 30, (String(packetsPerSecond) + "p/s").c_str());
        }
    } 
    else {
        // Nothing selected
        u8g2.drawStr(5, 35, "NO TARGET");
        u8g2.sendBuffer();
        return;
    }
    
    // Show attack status at bottom
    if (isDeauthRunning) {
        if (stationDeauthActive && apDeauthActive) {
            u8g2.drawStr(0, 60, "DEAUTH ACTIVE");
        } else if (stationDeauthActive) {
            u8g2.drawStr(0, 60, "STA DEAUTH");
        } else if (apDeauthActive) {
            u8g2.drawStr(0, 60, "AP DEAUTH");
        } else {
            u8g2.drawStr(0, 60, "DEAUTHING");
        }
    } else {
        u8g2.drawStr(0, 60, "STOPPED");
        packetsPerSecond = 0;  // Reset rate when stopped
    }
    
    u8g2.sendBuffer();
}
void drawStationSelectScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "STATIONS");
    u8g2.drawLine(0, 15, 127, 15);
    
    u8g2.setFont(u8g2_font_6x10_tf);  // smaller font
    
    if (scannedStationCount == 0) {
        u8g2.setFont(u8g2_font_8x13_tf);
        u8g2.drawStr(10, 35, "NO STATION");
    } else {
        for (int i = 0; i < 4 && i < scannedStationCount; i++) {
            int idx = i + stationSelectStartIdx;
            if (idx >= scannedStationCount) break;
            
            int y = 27 + (i * 11);
            
            if (idx == stationSelectIndex) {
                u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT-3, verticalBar);
            }
            
            if (stationSelected[idx]) {
                u8g2.drawXBM(7, y - 8, 8, 8, checkmark);
            } else {
                u8g2.drawFrame(7, y - 8, 8, 8);
            }
            
            // Build MAC string from raw bytes – no sscanf in hot loop
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     scannedStationMAC[idx][0], scannedStationMAC[idx][1],
                     scannedStationMAC[idx][2], scannedStationMAC[idx][3],
                     scannedStationMAC[idx][4], scannedStationMAC[idx][5]);
            u8g2.drawStr(20, y, macStr);
        }
    }
    
    u8g2.sendBuffer();
}
void drawStationScanScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    
    if (isScanningStations && !stationScanCancelled) {
        u8g2.drawStr(5, 12, "SCANNING STA");
        u8g2.drawLine(0, 15, 127, 15);
        u8g2.setFont(u8g2_font_8x13_tf);
        
        // ✅ DISPLAY PACKET COUNTER (Line 1)
        u8g2.drawStr(0, 30, "PKT:");
        u8g2.setCursor(40, 30);
        u8g2.print(stationScanPacketCount);
        
        // ✅ DISPLAY STATIONS FOUND (Line 2)
        u8g2.drawStr(0, 45, "STA:");
        u8g2.setCursor(40, 45);
        u8g2.print(scannedStationCount);
        
        // ✅ DISPLAY CURRENT AP (Line 3 - bottom)
        u8g2.setFont(u8g2_font_6x10_tf);
        String ssidToShow = currentScanSSID;
        if (ssidToShow.length() == 0) {
            ssidToShow = "<HIDDEN>";
        }
        
        // Handle long SSIDs with scrolling
        if (ssidToShow.length() > 18) {
            static String lastScrollingSSID = "";
            static int scrollPos = 0;
            static unsigned long lastScrollUpdate = 0;
            const unsigned long SCROLL_INTERVAL = 200;
            
            if (lastScrollingSSID != ssidToShow) {
                lastScrollingSSID = ssidToShow;
                scrollPos = 0;
                lastScrollUpdate = millis();
            }
            
            if (millis() - lastScrollUpdate > SCROLL_INTERVAL) {
                scrollPos++;
                if (scrollPos > ssidToShow.length() + 5) {
                    scrollPos = 0;
                }
                lastScrollUpdate = millis();
            }
            
            String scrollText = ssidToShow + "      ";
            String toShow = scrollText.substring(scrollPos, scrollPos + 18);
            if (toShow.length() < 18) {
                toShow += "            ";
                toShow = toShow.substring(0, 18);
            }
            u8g2.drawStr(0, 62, toShow.c_str());
        } else {
            u8g2.drawStr(0, 62, ssidToShow.c_str());
        }
        
    } else {
        // Scan complete screen
        u8g2.drawStr(5, 12, "SCAN COMPLETE");
        u8g2.drawLine(0, 15, 127, 15);
        u8g2.setFont(u8g2_font_8x13_tf);
        
        // ✅ SHOW FINAL PACKET COUNT
        u8g2.drawStr(0, 35, "PKTS:");
        u8g2.setCursor(45, 35);
        u8g2.print(stationScanPacketCount);
        
        // ✅ SHOW TOTAL STATIONS FOUND
        u8g2.drawStr(0, 50, "STAS:");
        u8g2.setCursor(45, 50);
        u8g2.print(scannedStationCount);
    }
    
    u8g2.sendBuffer();
}
void drawLogsScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "PASSWORD");
    u8g2.drawLine(0, 15, 127, 15);
    
    u8g2.setFont(u8g2_font_8x13_tf);
    
    if (passwordLogCount == 0) {
        u8g2.drawStr(10, 35, "NO LOGS");
    } else {
        // Show 4 logs
        for (int i = 0; i < 4; i++) {
            int idx = logScrollOffset + i;
            if (idx >= passwordLogCount) break;
            
            int y = 27 + (i * 11);
            
            if (idx == selectedLogIndex) {
                u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT-3, verticalBar);
            }
            
            if (passwordLogs[idx].verified) {
                u8g2.drawStr(7, y, "V");
            } else {
                u8g2.drawStr(7, y, "U");
            }
            
            String display = passwordLogs[idx].ssid.substring(0, 8) + ":" + passwordLogs[idx].password.substring(0, 4);
            u8g2.drawStr(18, y, display.c_str());
        }
        
        
    }
    
    u8g2.sendBuffer();
}
void drawLogDetailScreen() {
    if (selectedLogIndex >= passwordLogCount) return;
    
    PasswordLogEntry& entry = passwordLogs[selectedLogIndex];
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "LOG DETAIL");
    u8g2.drawLine(0, 15, 127, 15);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // BSSID (static, no scroll needed)
    u8g2.drawStr(0, 28, "BSSID:");
    String bssid = entry.bssid;
if (bssid.length() > 14) {
    static unsigned long lastScrollBSSID = 0;
    static int scrollPosBSSID = 0;
    if (millis() - lastScrollBSSID > 250) {
        scrollPosBSSID = (scrollPosBSSID + 1) % (bssid.length() + 3);
        lastScrollBSSID = millis();
    }
    String toShow = (bssid + "   ").substring(scrollPosBSSID, scrollPosBSSID + 14);
    u8g2.drawStr(45, 28, toShow.c_str());
} else {
    u8g2.drawStr(45, 28, bssid.c_str());
}
    
    // SSID with scrolling
    u8g2.drawStr(0, 40, "SSID:");
    String ssid = entry.ssid;
    if (ssid.length() > 14) {
        static unsigned long lastScrollSSID = 0;
        static int scrollPosSSID = 0;
        if (millis() - lastScrollSSID > 250) {
            scrollPosSSID = (scrollPosSSID + 1) % (ssid.length() + 3);
            lastScrollSSID = millis();
        }
        String toShow = (ssid + "   ").substring(scrollPosSSID, scrollPosSSID + 14);
        u8g2.drawStr(45, 40, toShow.c_str());
    } else {
        u8g2.drawStr(45, 40, ssid.c_str());
    }
    
    // Password with scrolling
    u8g2.drawStr(0, 52, "PASS:");
    String password = entry.password;
    if (password.length() > 12) {
        static unsigned long lastScrollPass = 0;
        static int scrollPosPass = 0;
        if (millis() - lastScrollPass > 250) {
            scrollPosPass = (scrollPosPass + 1) % (password.length() + 3);
            lastScrollPass = millis();
        }
        String toShow = (password + "   ").substring(scrollPosPass, scrollPosPass + 12);
        u8g2.drawStr(45, 52, toShow.c_str());
    } else {
        u8g2.drawStr(45, 52, password.c_str());
    }
    
    // Status
    u8g2.drawStr(0, 62, entry.verified ? "VERIFIED" : "UNVERIFIED");
    
    u8g2.sendBuffer();
}
void drawDeauthMonitorScanScreen() {
    unsigned long elapsed = millis() - scanStartTime;
    int progress = (elapsed * 100) / 20000;
    if (progress > 100) progress = 100;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "SCANNING");
    u8g2.drawLine(0, 15, 127, 15);
    
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(45, 35);
    u8g2.print(progress);
    u8g2.print("%");
    
    u8g2.drawFrame(10, 40, 108, 8);
    u8g2.drawBox(12, 42, (progress * 104) / 100, 4);
    
    u8g2.sendBuffer();
}
void drawDeauthMonitorScreen() {
    u8g2.clearBuffer();
    // Turn off LED after blink (using existing pulseState and lastPulseUpdate)
    if (pulseState && (millis() - lastPulseUpdate > 50)) {
        digitalWrite(LED_PIN, LOW);
        pulseState = false;
    }
    // CHANNEL at top
    u8g2.setFont(u8g2_font_7x13_tf);
    u8g2.drawStr(0, 10, "CH:");
    u8g2.setCursor(25, 10);
    if (currentMonitorChannel < 10) {
        u8g2.print("0");
        u8g2.print(currentMonitorChannel);
    } else {
        u8g2.print(currentMonitorChannel);
    }
    
    // PULSING DEAUTH! text
    bool recentActivity = false;
    unsigned long now = millis();
    int8_t latestRssi = -100;
    int8_t peakRssi = -100;
    
    for (int i = 0; i < deauthEventCount; i++) {
        if (now - deauthEvents[i].time < 500) {
            recentActivity = true;
            latestRssi = deauthEvents[i].rssi;
            break;
        }
    }
    
    // Find peak RSSI from ALL recent events (last 2 seconds)
    for (int i = 0; i < deauthEventCount; i++) {
        if (now - deauthEvents[i].time < 2000) {
            if (deauthEvents[i].rssi > peakRssi) {
                peakRssi = deauthEvents[i].rssi;
            }
        }
    }
    
    if (recentActivity) {
        if (now - lastPulseUpdate > 300) {
            pulseState = !pulseState;
            lastPulseUpdate = now;
        }
        if (pulseState) {
            u8g2.setFont(u8g2_font_8x13_tf);
            u8g2.drawStr(60, 10, "DEAUTH!");
        } else {
            u8g2.drawStr(60, 10, "       ");
        }
    } else {
        u8g2.drawStr(60, 10, "       ");
    }
    
    // Draw line below top row
    u8g2.drawLine(0, 14, 127, 14);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // Show latest deauth event
    if (deauthEventCount > 0) {
        int newestIdx = (deauthEventIndex - 1 + MAX_DEAUTH_EVENTS) % MAX_DEAUTH_EVENTS;
        DeauthEvent& ev = deauthEvents[newestIdx];
        
        if (now - ev.time < 1000) {
            // Show AP name or MAC - scrolling only for real SSIDs (not starting with "[")
            if (ev.apSsid != "" && !ev.apSsid.startsWith("[")) {
                // Real AP with SSID - show with scrolling if long
                String ssid = ev.apSsid;
                if (ssid.length() > 20) {
                    static String lastScrollingSSID = "";
                    static int scrollPos = 0;
                    static unsigned long lastScrollUpdate = 0;
                    const unsigned long SCROLL_INTERVAL = 200;
                    
                    if (lastScrollingSSID != ssid) {
                        lastScrollingSSID = ssid;
                        scrollPos = 0;
                        lastScrollUpdate = millis();
                    }
                    
                    if (millis() - lastScrollUpdate > SCROLL_INTERVAL) {
                        scrollPos++;
                        if (scrollPos > ssid.length() + 5) {
                            scrollPos = 0;
                        }
                        lastScrollUpdate = millis();
                    }
                    
                    String scrollText = ssid + "      ";
                    String toShow = scrollText.substring(scrollPos, scrollPos + 20);
                    if (toShow.length() < 20) {
                        toShow += "            ";
                        toShow = toShow.substring(0, 16);
                    }
                    u8g2.drawStr(0, 30, toShow.c_str());
                } else {
                    u8g2.drawStr(0, 30, ssid.c_str());
                }
            } else {
                // Hidden AP - show clean MAC (no brackets, no scrolling)
                String mac = ev.apMac;
                u8g2.drawStr(0, 30, mac.c_str());
            }
            
            // Show destination MAC below target
            u8g2.setCursor(0, 45);
            u8g2.print("DST:");
            u8g2.print(ev.stationMac);

            // Use:
            u8g2.setCursor(0, 60);
            u8g2.print("R:");
            if (now - ev.time < 1000) {
            u8g2.print(ev.reason);
        } else {
            u8g2.print("--");
        }
            
        } else {
            u8g2.drawStr(0, 30, "---");
        }
    } else {
        u8g2.drawStr(0, 30, "---");
    }
    
    // Add external variable declaration to access global counter
    extern int monitorDeauthPktsPerSecond;
    
    // Display packet rate at bottom right
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(75, 60);
    u8g2.print("D/s:");
    u8g2.print(monitorDeauthPktsPerSecond);
    
    u8g2.sendBuffer();
}
void drawClockScreen() {
    u8g2.clearBuffer();

    // Show "PS" in top right corner only when in power save mode
    if (powerSaveMode) {
        u8g2.setFont(u8g2_font_8x13_tf);
        u8g2.drawStr(108, 12, "PS");
    }
    
    if (timeSynced) {
        unsigned long elapsed = (millis() - lastTimeSync) / 1000;
        int hour, minute, second;
        sscanf(syncedTime.c_str(), "%d:%d:%d", &hour, &minute, &second);
        second += elapsed;
        minute += second / 60;
        second %= 60;
        hour += minute / 60;
        minute %= 60;
        hour %= 24;
        
        char displayTime[20];
        snprintf(displayTime, sizeof(displayTime), "%02d:%02d:%02d", hour, minute, second);
        
        u8g2.setFont(u8g2_font_fub20_tn);
        int w = u8g2.getStrWidth(displayTime);
        u8g2.drawStr((128 - w) / 2, 35, displayTime);
        
        u8g2.setFont(u8g2_font_8x13_tf);
        w = u8g2.getStrWidth(syncedDate.c_str());
        u8g2.drawStr((128 - w) / 2, 55, syncedDate.c_str());
    } else {
        // Show UPTIME instead of random time
        unsigned long uptimeSeconds = millis() / 1000;
        int hours = (uptimeSeconds % 86400) / 3600;
        int minutes = (uptimeSeconds % 3600) / 60;
        int seconds = uptimeSeconds % 60;
        
        char displayTime[20];
        snprintf(displayTime, sizeof(displayTime), "%02d:%02d:%02d", hours, minutes, seconds);
        
        u8g2.setFont(u8g2_font_fub20_tn);
        int w = u8g2.getStrWidth(displayTime);
        u8g2.drawStr((128 - w) / 2, 40, displayTime);
        
        // No extra label - keep it clean like the synced version
    }
    u8g2.sendBuffer();
}
void drawReactiveDeauthScreen() {
    u8g2.clearBuffer();
    // Turn off LED after blink
    if (pulseState && (millis() - lastPulseUpdate > 50)) {
        digitalWrite(LED_PIN, LOW);
        pulseState = false;
    }
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "DYNAMIC DEAUTH");
    u8g2.drawLine(0, 15, 127, 15);
    u8g2.setFont(u8g2_font_6x10_tf);
    
    if (reactiveDeauthActive) {
        String ssid = deauth_target_ssid.length() > 0 ? deauth_target_ssid : "<HIDDEN>";
        if (ssid.length() > 20) ssid = ssid.substring(0, 18) + "..";
        u8g2.drawStr(0, 25, ssid.c_str());
        
        // Show counters like "Assoc: 123  Auth: 456"
        u8g2.drawStr(0, 38, "Assoc:");
        u8g2.setCursor(40, 38);
        u8g2.print(reassoc_packet_count);
        
        u8g2.drawStr(65, 38, "Auth:");
        u8g2.setCursor(95, 38);
        u8g2.print(auth_packet_count);
        
        // Show deauths sent
        u8g2.drawStr(0, 50, "Deauth:");
        u8g2.setCursor(50, 50);
        u8g2.print(reactive_deauth_count);
        
        // MAC printing from your working version
        static unsigned long lastMacUpdate = 0;
        static String lastMac = "";
        
        if (currentClientMac != lastMac) {
            lastMac = currentClientMac;
            lastMacUpdate = millis();
        }
        
        if (millis() - lastMacUpdate < 2000) {
            u8g2.drawStr(0, 62, currentClientMac.c_str());
        } else {
            u8g2.drawStr(0, 62, "00:00:00:00:00:00");
            currentClientMac = "FF:FF:FF:FF:FF:FF";
        }
        
    } else {
        u8g2.drawStr(5, 35, "NO TARGET");
        u8g2.drawStr(5, 50, "Select AP first");
    }
    
    u8g2.sendBuffer();
}
void showNoTargetMessage() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 20, "NO TARGET");
    u8g2.drawLine(0, 23, 127, 23);
    u8g2.sendBuffer();
    delay(1500);
    startAccessPoint();
}
void drawMainMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "MAIN MENU");
    u8g2.drawLine(0, 15, 127, 15);
    
    u8g2.setFont(u8g2_font_8x13_tf);
    
    for (int i = 0; i < visibleItems; i++) {
        int idx = i + scrollOffset;
        if (idx >= mainMenuCount) break;
        
        int y = 27 + (i * 11);
        
        // Draw selection bar
        if (idx == selectedItemIndex) {
            u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT-3, verticalBar);
        }
        
        // Draw menu item text
        u8g2.drawStr(10, y, mainMenuItems[idx]);
        
        // Draw indicators for certain items
        if (idx == 1 && hasSelectedTargets()) {  // SELECT AP
            u8g2.drawStr(100, y, String(getSelectedTargetCount()).c_str());
        } 
        else if (idx == 4 && hasSelectedTargets()) {  // STA SCAN
            u8g2.drawStr(110, y, ">");
        } 
        else if (idx == 5 && getSelectedStationCount() > 0) {  // SELECT STA
            u8g2.drawStr(100, y, String(getSelectedStationCount()).c_str());
        } 
        else if (idx == 6 && passwordLogCount > 0) {  // LOGS
            u8g2.drawStr(100, y, String(passwordLogCount).c_str());
        }
    }
    
    u8g2.sendBuffer();
}