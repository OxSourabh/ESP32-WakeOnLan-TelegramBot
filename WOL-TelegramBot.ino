#include "WiFiMulti.h"
#include "WiFiClientSecure.h"
#include "WiFiUDP.h"
#include "WakeOnLan.h"
#include "UniversalTelegramBot.h"
#include "ArduinoJson.h"

// Configuration definitions
#define BOT_TOKEN "0000000000:000000000000000000000000000000000000"
#define ALLOWED_ID "000000000"
#define WIFI_SSID "<wifi-network-name>"
#define WIFI_PASS "<wifi-password>"
#define MAC_ADDR "00:00:00:00:00:00"

// Timing constants
const unsigned long BOT_MTBS = 5000;
const unsigned long RESTRART_MTBS = 1000 * 3600 * 4;
const unsigned long WIFI_TIMEOUT = 20000;  // 20 seconds timeout for WiFi connection
const int MAX_RETRIES = 3;  // Maximum number of retries for operations

// Status codes for error handling
enum ErrorCode {
  SUCCESS = 0,
  WIFI_CONNECTION_FAILED = 1,
  WOL_SEND_FAILED = 2,
  TELEGRAM_CONNECTION_FAILED = 3,
  NTP_SYNC_FAILED = 4,
  INVALID_MAC_FORMAT = 5
};

// Global variables
WiFiMulti wifiMulti;
WiFiClientSecure secured_client;
WiFiUDP UDP;
WakeOnLan WOL(UDP);
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;

// Error handling helper functions
String getErrorMessage(ErrorCode code) {
  switch (code) {
    case SUCCESS:
      return "Operation successful";
    case WIFI_CONNECTION_FAILED:
      return "Failed to connect to WiFi network";
    case WOL_SEND_FAILED:
      return "Failed to send Wake-on-LAN packet";
    case TELEGRAM_CONNECTION_FAILED:
      return "Failed to connect to Telegram servers";
    case NTP_SYNC_FAILED:
      return "Failed to synchronize time with NTP server";
    case INVALID_MAC_FORMAT:
      return "Invalid MAC address format";
    default:
      return "Unknown error occurred";
  }
}

void logError(ErrorCode code, const String& additional_info = "") {
  Serial.print("ERROR [");
  Serial.print(code);
  Serial.print("]: ");
  Serial.println(getErrorMessage(code));
  if (additional_info.length() > 0) {
    Serial.println("Additional info: " + additional_info);
  }
}

// Validate MAC address format
bool isValidMacAddress(const char* mac) {
  if (strlen(mac) != 17) return false;
  
  for (int i = 0; i < 17; i++) {
    if (i % 3 == 2) {
      if (mac[i] != ':') return false;
    } else {
      if (!isxdigit(mac[i])) return false;
    }
  }
  return true;
}

// Enhanced WoL function with retry mechanism
ErrorCode sendWOL() {
  if (!isValidMacAddress(MAC_ADDR)) {
    logError(INVALID_MAC_FORMAT);
    return INVALID_MAC_FORMAT;
  }

  for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
    if (WiFi.status() != WL_CONNECTED) {
      logError(WIFI_CONNECTION_FAILED, "Reconnecting to WiFi...");
      if (!connectToWiFi()) {
        continue;
      }
    }

    bool success = WOL.sendMagicPacket(MAC_ADDR);
    if (success) {
      Serial.println("Magic packet sent successfully");
      return SUCCESS;
    }
    
    logError(WOL_SEND_FAILED, "Attempt " + String(attempt + 1) + " of " + String(MAX_RETRIES));
    delay(1000 * (attempt + 1));  // Exponential backoff
  }
  
  return WOL_SEND_FAILED;
}

// Enhanced WiFi connection function
bool connectToWiFi() {
  unsigned long startAttemptTime = millis();
  
  Serial.print("Connecting to WiFi...");
  while (wifiMulti.run() != WL_CONNECTED) {
    if (millis() - startAttemptTime > WIFI_TIMEOUT) {
      Serial.println("\nFailed to connect to WiFi within timeout period");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

// Enhanced message handling with error reporting
void handleNewMessages(int numNewMessages) {
  Serial.print("Processing ");
  Serial.print(numNewMessages);
  Serial.println(" new messages");

  for (int i = 0; i < numNewMessages; i++) {
    Serial.println("Checking message from: " + bot.messages[i].from_id);
    
    if (bot.messages[i].from_id != ALLOWED_ID) {
      Serial.println("Unauthorized access attempt from ID: " + bot.messages[i].from_id);
      continue;
    }

    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name.length() > 0 ? 
                      bot.messages[i].from_name : "Guest";

    if (text == "/wol") {
      ErrorCode result = sendWOL();
      if (result == SUCCESS) {
        bot.sendMessage(chat_id, "Magic Packet sent successfully!", "");
      } else {
        String errorMsg = "Failed to send Magic Packet: " + getErrorMessage(result);
        bot.sendMessage(chat_id, errorMsg, "");
      }
    } else {
      String welcome = "Welcome to *WoL Bot*, " + from_name + ".\n";
      welcome += "/wol : Send the Magic Packet\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

// Enhanced setup with proper error handling
void setup() {
  Serial.begin(9600);
  Serial.println("\nInitializing WoL Bot...");

  // Configure WiFi
  wifiMulti.addAP(WIFI_SSID, WIFI_PASS);
  if (!connectToWiFi()) {
    logError(WIFI_CONNECTION_FAILED);
    ESP.restart();
    return;
  }

  // Configure secure client
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  
  // Configure WoL
  WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
  Serial.println("WoL configured with broadcast address: " + WiFi.localIP().toString());

  // Sync time
  Serial.print("Synchronizing time...");
  configTime(0, 0, "pool.ntp.org");
  time_t now = time(nullptr);
  unsigned long startAttemptTime = millis();
  
  while (now < 24 * 3600) {
    if (millis() - startAttemptTime > WIFI_TIMEOUT) {
      logError(NTP_SYNC_FAILED);
      ESP.restart();
      return;
    }
    delay(150);
    Serial.print(".");
    now = time(nullptr);
  }
  
  Serial.println("\nTime synchronized successfully");
  Serial.println("Bot initialization complete");
}

// Enhanced main loop with error handling
void loop() {
  // Check WiFi connection and reconnect if necessary
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    if (!connectToWiFi()) {
      logError(WIFI_CONNECTION_FAILED);
      delay(5000);  // Wait before next attempt
      return;
    }
  }

  // Check for new Telegram messages
  if (millis() - bot_lasttime > BOT_MTBS) {
    try {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      
      while (numNewMessages) {
        Serial.println("New messages received");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      
      bot_lasttime = millis();
    } catch (...) {
      logError(TELEGRAM_CONNECTION_FAILED);
      delay(1000);
    }
  }

  // Check if it's time to restart
  if (millis() > RESTRART_MTBS) {
    Serial.println("Performing scheduled restart");
    ESP.restart();
  }

  delay(100);
}