#include "BeaconSpammer.h"

// Define static members
const uint8_t BeaconSpammer::channels[3] = {1, 6, 11};

// Empty SSID list - we don't need this anymore but keeping for structure
const char BeaconSpammer::ssids[] PROGMEM = "";

// Global instance pointer
BeaconSpammer* beaconSpammer = nullptr;

// Character sets for random SSID generation
const char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -_[]()";
const char words[][10] = {
    "WiFi", "NETWORK", "HOME", "GUEST", "LINKSYS", "NETGEAR", "TP-LINK",
    "FBI", "NSA", "CIA", "PHONE", "IPHONE", "ANDROID", "SAMSUNG",
    "FREE", "PUBLIC", "PRIVATE", "SECURE", "HACKED", "ROUTER", "MODEM"
};

// Constructor
BeaconSpammer::BeaconSpammer(bool enableWPA2, bool appendSpaces) {
    wpa2_enabled = enableWPA2;
    append_spaces = appendSpaces;
    running = false;
    channelIndex = 0;
    wifi_channel = 1;
    packetCounter = 0;
    attackTime = 0;
    packetRateTime = 0;
    
    // Initialize beacon packet
    uint8_t tempPacket[109] = {
        0x80, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
        0x00, 0x00, 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00,
        0xe8, 0x03, 0x31, 0x00,
        0x00, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c,
        0x03, 0x01, 0x01,
        0x30, 0x18, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02,
        0x02, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04,
        0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00
    };
    
    memcpy(beaconPacket, tempPacket, 109);
    
    // Set packet size based on WPA2 setting
    packetSize = sizeof(beaconPacket);
    if (!wpa2_enabled) {
        beaconPacket[34] = 0x21;
        packetSize -= 26;
    }
    
    // Create empty SSID
    for (int i = 0; i < 32; i++) {
        emptySSID[i] = ' ';
    }
}

void BeaconSpammer::begin() {
    randomSeed(analogRead(0));
    randomMac();
    
    // FULL cleanup of previous WiFi state
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    esp_wifi_deinit();
    delay(100);
    
    // Fresh init for STA mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    delay(100);
    
    WiFi.mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_ps(WIFI_PS_NONE);
    delay(200);
    
    esp_wifi_set_channel(channels[0], WIFI_SECOND_CHAN_NONE);
    
    Serial.println("Beacon Spammer initialized");
}

void BeaconSpammer::start() {
    running = true;
    attackTime = millis();
    packetRateTime = millis();
    packetCounter = 0;
    Serial.println("Beacon Spammer started in RANDOM mode");
}

void BeaconSpammer::stop() {
    running = false;
    Serial.println("Beacon Spammer stopped");
}

void BeaconSpammer::nextChannel() {
    if (sizeof(channels) > 1) {
        uint8_t ch = channels[channelIndex];
        channelIndex++;
        if (channelIndex >= sizeof(channels)) channelIndex = 0;
        
        if (ch != wifi_channel && ch >= 1 && ch <= 14) {
            wifi_channel = ch;
            esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
        }
    }
}

void BeaconSpammer::randomMac() {
    for (int i = 0; i < 6; i++) {
        macAddr[i] = random(256);
    }
}

void BeaconSpammer::generateRandomSSID(char* ssid, uint8_t& len) {
    // Generate random length between 5 and 25 characters
    len = random(5, 26);
    
    // 30% chance to use word-based SSID
    if (random(100) < 30) {
        int wordCount = sizeof(words) / sizeof(words[0]);
        int wordIndex = random(wordCount);
        String word = String(words[wordIndex]);
        
        // Add random suffix sometimes
        if (random(100) < 40) {
            word += "-" + String(random(1000, 9999));
        }
        
        len = min((int)word.length(), 32);
        for (int i = 0; i < len; i++) {
            ssid[i] = word[i];
        }
    } else {
        // Generate completely random SSID
        int lettersLen = sizeof(letters) - 1; // exclude null terminator
        for (int i = 0; i < len; i++) {
            ssid[i] = letters[random(lettersLen)];
        }
    }
}

void BeaconSpammer::update() {
    if (!running) return;
    
    uint32_t currentTime = millis();
    
    if (currentTime - attackTime > 20) {
        attackTime = currentTime;
        
        nextChannel();
        randomMac();
        
        // Generate and send multiple SSIDs per cycle
        for (int i = 0; i < 15; i++) {
            char ssid[33];
            uint8_t ssidLen;
            
            // ALWAYS generate random SSID
            generateRandomSSID(ssid, ssidLen);
            
            macAddr[5] = i; // Vary MAC for each SSID
            
            memcpy(&beaconPacket[10], macAddr, 6);
            memcpy(&beaconPacket[16], macAddr, 6);
            memcpy(&beaconPacket[38], emptySSID, 32);
            memcpy(&beaconPacket[38], ssid, ssidLen);
            beaconPacket[82] = wifi_channel;
            
            if (append_spaces) {
                if (esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, packetSize, 0) == 0) {
                    packetCounter++;
                }
            } else {
                uint16_t tmpPacketSize = (109 - 32) + ssidLen;
                uint8_t* tmpPacket = new uint8_t[tmpPacketSize];
                memcpy(&tmpPacket[0], &beaconPacket[0], 37 + ssidLen);
                tmpPacket[37] = ssidLen;
                memcpy(&tmpPacket[38 + ssidLen], &beaconPacket[70], 39);
                
                if (esp_wifi_80211_tx(WIFI_IF_STA, tmpPacket, tmpPacketSize, 0) == 0) {
                    packetCounter++;
                }
                delete[] tmpPacket;
            }
        }
    }
    
    if (currentTime - packetRateTime > 1000) {
        packetRateTime = currentTime;
        Serial.print("Beacon packets/s: ");
        Serial.println(packetCounter);
        packetCounter = 0;
    }
}