#ifndef BEACON_SPAMMER_H
#define BEACON_SPAMMER_H

#include <Arduino.h>
#include <WiFi.h>

extern "C" {
#include "esp_wifi.h"
}

class BeaconSpammer {
private:
    static const uint8_t channels[3];
    uint8_t channelIndex;
    uint8_t wifi_channel;
    
    uint8_t macAddr[6];
    
    uint32_t packetCounter;
    uint32_t attackTime;
    uint32_t packetRateTime;
    
    char emptySSID[32];
    
    uint8_t beaconPacket[109];
    uint16_t packetSize;
    
    bool wpa2_enabled;
    bool append_spaces;
    
    static const char ssids[] PROGMEM;
    
    void nextChannel();
    void randomMac();
    void generateRandomSSID(char* ssid, uint8_t& len);
    
public:
    BeaconSpammer(bool enableWPA2 = true, bool appendSpaces = true);
    
    void begin();
    void update();
    void start();
    void stop();
    bool isRunning() { return running; }
    uint32_t getPacketRate() { return packetCounter; }
    
    bool running;
};

extern BeaconSpammer* beaconSpammer;

#endif