// ========== AC/LIVE WIRE DETECTOR ==========

const int acSensePin = 34;
const int acSamplesPerCycle = 20;
const int acNumCycles = 5;
const int acTotalSamples = acSamplesPerCycle * acNumCycles;

int acSampleBuffer[100];
int acSampleIndex = 0;
bool acSamplingComplete = false;
unsigned long acLastSampleTime = 0;  // Using millis() like standalone

float acFrequency = 0;
int acAmplitude = 0;
float acRms = 0;
float acEdgeSharpness = 0;
String acStatusMessage = "";
bool acLiveWire = false;

void initACDetector() {
    // Disable everything
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    setCpuFrequencyMhz(240);
    
    acSampleIndex = 0;
    acSamplingComplete = false;
    acLastSampleTime = 0;
    acFrequency = 0;
    acAmplitude = 0;
    acRms = 0;
    acEdgeSharpness = 0;
    acLiveWire = false;
    acStatusMessage = "";
    
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);
    pinMode(acSensePin, INPUT);
    
    // Use default LED_PIN (GPIO 2)
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void updateACDetector() {
    unsigned long currentTime = millis();
    
    // EXACT same sampling as standalone - using millis()
    if (!acSamplingComplete && (currentTime - acLastSampleTime >= 1)) {
        acLastSampleTime = currentTime;
        
        if (acSampleIndex < acTotalSamples) {
            acSampleBuffer[acSampleIndex] = analogRead(acSensePin);
            acSampleIndex++;
        } else {
            acSamplingComplete = true;
        }
    }
    
    // Process when buffer is full
    if (acSamplingComplete) {
        // Calculate zero crossings
        int zeroCrossings = 0;
        for (int i = 1; i < acTotalSamples; i++) {
            if ((acSampleBuffer[i-1] < 2048 && acSampleBuffer[i] >= 2048) ||
                (acSampleBuffer[i-1] > 2048 && acSampleBuffer[i] <= 2048)) {
                zeroCrossings++;
            }
        }
        
        acFrequency = (zeroCrossings / 2.0) / 0.1;
        
        // Calculate amplitude
        int maxVal = 0, minVal = 4095;
        for (int i = 0; i < acTotalSamples; i++) {
            if (acSampleBuffer[i] > maxVal) maxVal = acSampleBuffer[i];
            if (acSampleBuffer[i] < minVal) minVal = acSampleBuffer[i];
        }
        acAmplitude = maxVal - minVal;
        
        // Calculate slope
        int maxSlope = 0;
        for (int i = 1; i < acTotalSamples; i++) {
            int slope = abs(acSampleBuffer[i] - acSampleBuffer[i-1]);
            if (slope > maxSlope) maxSlope = slope;
        }
        
        acEdgeSharpness = 0;
        if (acAmplitude > 0) {
            acEdgeSharpness = (maxSlope * 100.0) / acAmplitude;
        }
        
        // Calculate RMS
        long sumSquares = 0;
        for (int i = 0; i < acTotalSamples; i++) {
            int dcOffset = acSampleBuffer[i] - 2048;
            sumSquares += (long)dcOffset * dcOffset;
        }
        acRms = sqrt(sumSquares / (float)acTotalSamples);
        
        // Detection logic
        bool is50Hz = (acFrequency >= 45 && acFrequency <= 60);
        bool hasSignal = (acAmplitude > 100);
        bool isSharp = (acEdgeSharpness > 12);
        
        unsigned long currentMillis = millis();
        
        if (is50Hz && hasSignal) {
            if (isSharp) {
                acStatusMessage = "LIVE WIRE!";
                acLiveWire = true;
                // Use default LED_PIN (GPIO 2) for blinking
                static unsigned long lastBlink = 0;
                if (currentMillis - lastBlink > 10) {
                    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                    lastBlink = currentMillis;
                }
            } else {
                acStatusMessage = "NEUTRAL";
                acLiveWire = false;
                digitalWrite(LED_PIN, LOW);
            }
        } 
        else if (is50Hz && acAmplitude > 50 && acAmplitude <= 100) {
            acStatusMessage = "WEAK";
            acLiveWire = false;
            digitalWrite(LED_PIN, LOW);
        }
        else {
            acStatusMessage = "NO AC";
            acLiveWire = false;
            digitalWrite(LED_PIN, LOW);
        }
        
        // Reset for next reading
        acSampleIndex = 0;
        acSamplingComplete = false;
    }
}

void drawACDetectorScreen() {
    u8g2.clearBuffer();
    
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "AC DETECTOR");
    u8g2.drawLine(0, 15, 127, 15);
    
    u8g2.setFont(u8g2_font_6x10_tf);
    char line1[32];
    snprintf(line1, sizeof(line1), "Freq:%.1fHz  Amp:%d", acFrequency, acAmplitude);
    u8g2.drawStr(0, 28, line1);
    
    char line2[32];
    snprintf(line2, sizeof(line2), "RMS:%.0f", acRms);
    u8g2.drawStr(0, 40, line2);
    
    u8g2.setFont(u8g2_font_8x13_tf);
    if (acLiveWire) {
        u8g2.drawStr(0, 55, "[AC DETECTED]");
        
    } else {
        u8g2.drawStr(0, 55, acStatusMessage.c_str());
        if (acStatusMessage == "NEUTRAL") {
            u8g2.drawStr(0, 68, "[SMOOTH]");
        } else if (acStatusMessage == "WEAK") {
            u8g2.drawStr(0, 68, "MOVE CLOSER");
        }
    }
    
    u8g2.sendBuffer();
}

void runACDetectorLoop() {
    unsigned long lastBackCheck = 0;
    bool wasApRunning = apAndWebRunning;
    
    // Stop everything
    if (wasApRunning) {
        webServer.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apAndWebRunning = false;
        webServerRunning = false;
    }
    
    apDeauthActive = false;
    stationDeauthActive = false;
    combinedDeauthActive = false;
    flood_active = false;
    deauthAllActive = false;
    reactiveDeauthActive = false;
    if (beaconSpam.running) beaconSpam.stop();
    if (deauthMonitorActive) stopDeauthMonitor();
    if (isScanningStations) stopStationScan();
    if (hiddenRecoveryActive) stopHiddenRecovery();
    
    initACDetector();
    
    Serial.println("AC Detector Active - Press BACK to exit");
    
    // Allow sampling to stabilize
    delay(500);
    
    // Clear display once
    drawACDetectorScreen();
    
    // MAIN LOOP - EXACTLY LIKE STANDALONE
    while (true) {
        updateACDetector();
        
        // Update display every 200ms (like standalone's delay)
        static unsigned long lastDisplayTime = 0;
        if (millis() - lastDisplayTime >= 200) {
            drawACDetectorScreen();
            lastDisplayTime = millis();
        }
        
        // Check BACK button
        if (digitalRead(BUTTON_BACK) == LOW) {
            if (millis() - lastBackCheck > 200) {
                lastBackCheck = millis();
                break;
            }
        }
        
        // NO delay here - let updateACDetector control timing
        // Just yield to prevent watchdog
        delayMicroseconds(100);
    }
    
    // Turn off LED when exiting
    digitalWrite(LED_PIN, LOW);
    
    startAccessPoint();
}