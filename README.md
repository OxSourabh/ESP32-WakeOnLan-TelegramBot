# ESP32 Wake-on-LAN Telegram-Bot


This code implements a Telegram bot that can remotely wake up a computer using Wake-on-LAN (WoL) functionality. The program runs on an ESP8266/ESP32 microcontroller and acts as a bridge between Telegram and the local network.

## Key Components

### Libraries and Dependencies
- WiFiMulti: Handles WiFi connection management
- WiFiClientSecure: Provides secure HTTPS connections
- WiFiUDP: Handles UDP communication for WoL
- WakeOnLan: Implements WoL functionality
- UniversalTelegramBot: Interfaces with Telegram Bot API
- ArduinoJson: Handles JSON parsing (though not explicitly used in the visible code)

### Configuration
```
#define BOT_TOKEN "0000000000:000000000000000000000000000000000000"
#define ALLOWED_ID "000000000"
#define WIFI_SSID "<wifi-network-name>"
#define WIFI_PASS "<wifi-password>"
#define MAC_ADDR "00:00:00:00:00:00"- Implements basic security by allowing only one specific Telegram user ID
```
- Requires configuration of WiFi credentials and target device's MAC address
- Bot token needs to be obtained from Telegram's BotFather
### Pairing Bot


### Timing Intervals
```
const unsigned long BOT_MTBS = 5000;  // Message check interval (5 seconds)
const unsigned long RESTRART_MTBS = 1000 * 3600 * 4;  // Restart every 4 hours
```
## Main Functions

### setup()
1. Initializes serial communication
2. Connects to WiFi
3. Sets up secure client with Telegram's certificate
4. Configures WoL broadcast address
5. Synchronizes time with NTP server

### loop()
1. Checks for new Telegram messages every 5 seconds
2. Handles any new messages through handleNewMessages()
3. Implements automatic restart every 4 hours

### handleNewMessages()
1. Validates sender's ID against ALLOWED_ID
2. Processes two commands:
   - /wol: Sends WoL magic packet
   - Any other input: Sends welcome message with instructions

### sendWOL()
Simple function that sends the magic packet to the specified MAC address with a 300ms delay


### Potential Improvements
1. MAC address could be stored in more secure manner
2. Could implement additional authentication methods
3. Could add logging of wake attempts
4. Could add rate limiting for WoL requests

## Features and UseCase
1. Automatic restart every 4 hours to prevent memory issues
2. WiFiMulti for better WiFi connection handling
3. Proper time synchronization before starting operations

## Future Upgrades RoadMap
1. Error handling could be more robust
2. Could add status reporting functionality
3. Could implement multiple MAC address support
4. Could add network status monitoring
5. Could implement power state detection of target machine
6. Missing explicit error messages for failed operations
7. No retry mechanism for failed WoL attempts

