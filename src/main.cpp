// Core Arduino and system includes
#include <Arduino.h>

// System libraries
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>
#include <time.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <FS.h>
#include <SD_MMC.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <ElegantOTA.h> // ElegantOTA works with WebServer

// Hardware-specific libraries
#include <Arduino_GFX_Library.h>

// LVGL and display helpers
#include <Arduino_GFX_Library.h>
#include <TAMC_GT911.h>

// Audio components
#include <AudioOutputI2S.h>
#include <AudioGeneratorMP3.h>
#include <AudioFileSourcePROGMEM.h>
#include <AudioFileSourceHTTPStream.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceBuffer.h>

// Display manager
#include "DisplayManager.h"
#include <Adafruit_SGP30.h>
#include <Adafruit_SHT31.h>
#include "UIManager.h"
#include "ConfigManager.h"
#include "AudioManager.h"
#include "AlarmManager.h"

// Backlight control
#define BACKLIGHT_PIN 44  // Backlight control pin (PWM)

// I2C for sensors
#define I2C_SDA 38
#define I2C_SCL 37

// SD Card
#define SD_MMC_CMD 13
#define SD_MMC_CLK 12
#define SD_MMC_D0 11

// Audio
#define I2S_DOUT 10
#define I2S_BCLK 12
#define I2S_LRC 13

// Global objects
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
Preferences preferences;

// Global display manager instance (forward declaration)
class DisplayManager;
DisplayManager& displayManager = DisplayManager::getInstance();

// Forward declarations
void wifi_init();
void time_init();
void sdcard_init();
void sensors_init();
void audio_init();
void web_server_init();
void update_display_task(void *parameter);
void update_sensors_task(void *parameter);
void check_alarms_task(void *parameter);
void update_weather_task(void *parameter);

// Forward declarations for manager classes
#include "DisplayManager.h"
#include "UIManager.h"
#include "ConfigManager.h"
#include "AudioManager.h"
#include "AlarmManager.h"
#include "WeatherService.h"

// Task handles
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t lvglTaskHandle = nullptr;
TaskHandle_t sensorsTaskHandle = NULL;
TaskHandle_t alarmTaskHandle = NULL;
TaskHandle_t weatherTaskHandle = NULL;

// LVGL timer for settings screen timeout
lv_timer_t* settingsTimeoutTimer = NULL;
// Global tracking of last touch time for screen timeout
unsigned long lastTouchTime = 0;

Adafruit_SGP30 sgp;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Web server
WebServer server(80);

// LVGL display buffer is now managed by DisplayManager

// LVGL callback for handling button events
static void screen_change_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    
    if (code == LV_EVENT_CLICKED) {
        // Get the screen type from user_data
        uint32_t *screen_type = (uint32_t *)lv_event_get_user_data(e);
        if (!screen_type) {
            Serial.println("Error: No screen type data");
            return;
        }
        
        Serial.printf("Screen button clicked, type: %d\n", *screen_type);
        
        UIManager& ui = UIManager::getInstance();
        switch (*screen_type) {
            case 0: // Home screen
                Serial.println("==== NAVIGATION: -> HOME (via button) ====");
                ui.showHomeScreen();
                break;
            case 1: // Settings screen
                Serial.println("==== NAVIGATION: -> SETTINGS (via button) ====");
                ui.showSettingsScreen();
                break;
            case 2: // Alarm screen
                Serial.println("==== NAVIGATION: -> ALARM (via button) ====");
                ui.showAlarmSettingsScreen();
                break;
            case 3: // Radio screen
                Serial.println("==== NAVIGATION: -> RADIO (via button) ====");
                ui.showRadioScreen();
                break;
            default:
                Serial.printf("Unknown screen type: %d\n", *screen_type);
                break;
        }
    }
}

// Define timeout and debounce constants
static const uint32_t settingsTimeoutDelay = 10000; // 10 seconds timeout for settings
static const uint32_t debounceDelay = 500; // 500ms debounce between screen changes
static uint32_t lastScreenChange = 0;

// Callback for touchscreen events
static void touchscreen_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    
    // Only handle press and release events
    if (code == LV_EVENT_PRESSED) {
        // Update last touch time for timeout calculation
        lastTouchTime = millis();
        Serial.println("Touch detected - updating timeout timer");
    }
}

// Function declarations
void setup();
void loop();
void lvgl_init();
void wifi_init();
void time_init();
void sdcard_init();
void sensors_init();
void audio_init();
void web_server_init();
void lvgl_tick(void *parameter);
void update_display_task(void *parameter);
void update_sensors_task(void *parameter);
void check_alarms_task(void *parameter);

// Alarm triggered callback
void onAlarmTriggered(const Alarm& alarm) {
    // Get singleton instances
    UIManager& ui = UIManager::getInstance();
    AudioManager& audio = AudioManager::getInstance();
    
    // Show alarm screen
    ui.showAlarmScreen();
    
    // If it's a radio alarm, start playing the radio
    if (alarm.source == 0) { // Radio
        // Use playStream instead as there's no direct playRadio method
        // Get the URL for the station index
        String stationUrl = "http://example.com/radio.mp3"; // You should get this from a station list
        audio.playStream(stationUrl.c_str());
    } 
    // If it's an MP3 alarm, play the specified file
    else if (alarm.source == 1) { // MP3
        audio.playFile(alarm.sourceData.filepath);
    }
    
    // Set volume using the AudioManager singleton instance
    audio.setVolume(alarm.volume);
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    delay(1000); // Add delay before any debug output
    Serial.println("\n\n[DEBUG] *** BOOT STARTED ***");
    Serial.println("[DEBUG] Serial initialized");
    // Don't wait for serial port as it might cause a hang
    // while (!Serial);
    
    // Initialize file system
    Serial.println("[DEBUG] Starting SPIFFS initialization...");
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] SPIFFS initialization failed!");
        Serial.println("[DEBUG] Entering error loop");
        while (1) {
            delay(1000);
            Serial.println("[ERROR] SPIFFS init failed - stuck in error loop");
        }
    }
    Serial.println("[DEBUG] SPIFFS initialized successfully");
    
    // Get singleton instances
    Serial.println("[DEBUG] Getting ConfigManager instance...");
    ConfigManager& config = ConfigManager::getInstance();
    
    Serial.println("[DEBUG] Getting DisplayManager instance...");
    DisplayManager& display = DisplayManager::getInstance();
    
    Serial.println("[DEBUG] Getting UIManager instance...");
    UIManager& ui = UIManager::getInstance();
    
    Serial.println("[DEBUG] Getting AlarmManager instance...");
    AlarmManager& alarm = AlarmManager::getInstance();
    
    // Initialize configuration
    Serial.println("[DEBUG] Starting ConfigManager initialization...");
    if (!config.begin()) {
        Serial.println("[ERROR] ConfigManager initialization failed!");
        Serial.println("[DEBUG] Entering error loop");
        while (1) {
            delay(1000);
            Serial.println("[ERROR] ConfigManager init failed - stuck in error loop");
        }
    }
    Serial.println("[DEBUG] ConfigManager initialized successfully");
    
    // Initialize display
    Serial.println("[DEBUG] Starting DisplayManager initialization...");
    if (!display.begin()) {
        Serial.println("[ERROR] DisplayManager initialization failed!");
        Serial.println("[DEBUG] Entering error loop");
        while (1) {
            delay(1000);
            Serial.println("[ERROR] DisplayManager init failed - stuck in error loop");
        }
    }
    Serial.println("[DEBUG] DisplayManager initialized successfully");
    
    // Initialize UI
    Serial.println("[DEBUG] Starting UIManager initialization...");
    if (!ui.init()) {
        Serial.println("[ERROR] UIManager initialization failed!");
        Serial.println("[DEBUG] Entering error loop");
        while (1) {
            delay(1000);
            Serial.println("[ERROR] UIManager init failed - stuck in error loop");
        }
    }
    Serial.println("[DEBUG] UIManager initialized successfully");
    
    // Initialize WiFi
    Serial.println("[DEBUG] Starting WiFi initialization...");
    wifi_init();
    Serial.println("[DEBUG] WiFi initialization completed");
    
    // Initialize time
    Serial.println("[DEBUG] Starting time initialization...");
    time_init();
    Serial.println("[DEBUG] Time initialization completed");
    
    // Initialize SD card
    Serial.println("[DEBUG] Starting SD card initialization...");
    sdcard_init();
    Serial.println("[DEBUG] SD card initialization completed");
    
    // Initialize sensors
    Serial.println("[DEBUG] Starting sensors initialization...");
    sensors_init();
    Serial.println("[DEBUG] Sensors initialization completed");
    
    // Initialize audio
    Serial.println("[DEBUG] Starting audio initialization...");
    audio_init();
    Serial.println("[DEBUG] Audio initialization completed");
    
    // Initialize web server
    Serial.println("[DEBUG] Starting web server initialization...");
    web_server_init();
    Serial.println("[DEBUG] Web server initialization completed");
    
    // Create tasks for updating display, reading sensors, and checking alarms
    Serial.println("[DEBUG] Creating FreeRTOS tasks...");
    
    Serial.println("[DEBUG] Creating DisplayTask on core 0");
    BaseType_t displayTaskCreated = xTaskCreatePinnedToCore(
        update_display_task,   // Task function
        "DisplayTask",        // Task name for debugging
        4096,                 // Stack size (in words)
        NULL,                 // Task parameters
        2,                    // Task priority
        &displayTaskHandle,   // Task handle
        0                     // Core to run the task on (core 0)
    );
    
    BaseType_t sensorsTaskCreated = xTaskCreatePinnedToCore(
        update_sensors_task,   // Task function
        "SensorsTask",         // Task name for debugging
        4096,                  // Stack size (in words)
        NULL,                  // Task parameters
        1,                     // Task priority
        &sensorsTaskHandle,    // Task handle
        1                      // Core to run the task on (core 1)
    );
    
    BaseType_t alarmsTaskCreated = xTaskCreate(
        check_alarms_task,     // Function
        "AlarmTask",          // Name
        4096,                 // Stack size
        NULL,                 // Parameters
        1,                    // Priority
        &alarmTaskHandle      // Task handle - use the correct variable name
    );
    
    // Create weather update task
    Serial.println("[DEBUG] Creating WeatherTask on core 1");
    BaseType_t weatherTaskCreated = xTaskCreatePinnedToCore(
        update_weather_task,  // Task function
        "WeatherTask",       // Task name for debugging
        8192,                // Stack size (in words) - larger for JSON parsing
        NULL,                // Task parameters
        1,                   // Task priority
        &weatherTaskHandle,  // Task handle
        1                    // Core to run the task on (core 1)
    );
    
    // Check if all tasks were created successfully
    if (displayTaskCreated != pdPASS || 
        sensorsTaskCreated != pdPASS || 
        alarmsTaskCreated != pdPASS ||
        weatherTaskCreated != pdPASS) {
        
        Serial.println("Error: Failed to create one or more tasks!");
        while (1) { delay(1000); } // Halt if tasks can't be created
    }
    
    Serial.println("Setup complete - System is running");
    Serial.println("------------------------------------");
    
    // Show home screen using the singleton instance
    ui.showHomeScreen();
    
    // Initialize the settings screen timeout timer
    settingsTimeoutTimer = lv_timer_create([](lv_timer_t* timer) {
        // Check if we're on the settings screen and it's been more than 10 seconds since last touch
        UIManager& ui = UIManager::getInstance();
        if (ui.settingsScreen && lv_scr_act() == ui.settingsScreen && millis() - lastTouchTime > 10000) {
            Serial.println("Settings screen timeout - returning to home screen");
            ui.showHomeScreen();
        }
    }, 1000, NULL); // Check every second
    
    Serial.println("Setup complete");
}

void loop() {
    static bool firstLoop = true;
    if (firstLoop) {
        Serial.println("[DEBUG] First loop iteration");
        firstLoop = false;
    }
    
    // Get UIManager instance
    UIManager& ui = UIManager::getInstance();
    
    // Update UI is handled separately via tasks so no need to call update() here
    
    // Update time string once per second
    static unsigned long lastTimeUpdate = 0;
    static char lastTimeStr[9] = "";
    static char lastDateStr[32] = "";
    static uint8_t updateCounter = 0;
    
    unsigned long now = millis();
    if (now - lastTimeUpdate >= 1000) {
        lastTimeUpdate = now;
        updateCounter++;
        
        // Only print memory info every 10 seconds
        if (updateCounter % 10 == 0) {
            Serial.printf("[MEM] Free heap: %d bytes\n", ESP.getFreeHeap());
        }
        
        // Get current time
        struct tm timeinfo;
        char timeStr[9];  // HH:MM:SS + null terminator
        char dateStr[32];
        
        if (getLocalTime(&timeinfo)) {
            // Format time as HH:MM:SS
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
            // Create German weekday array
            const char* germanWeekdays[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
            // Format date with German weekday and dd.mm.yyyy format
            char formattedDate[32];
            strftime(formattedDate, sizeof(formattedDate), "%d.%m.%Y", &timeinfo);
            snprintf(dateStr, sizeof(dateStr), "%s %s", germanWeekdays[timeinfo.tm_wday], formattedDate);
            
            // Only update if the time or date has changed
            if (strcmp(timeStr, lastTimeStr) != 0) {
                strcpy(lastTimeStr, timeStr);
                ui.updateTime(timeStr);
            }
            
            if (strcmp(dateStr, lastDateStr) != 0) {
                strcpy(lastDateStr, dateStr);
                ui.updateDate(dateStr);
            }
        }
    }
    
    // Small delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(10));
}

void wifi_init() {
    // Get ConfigManager instance
    ConfigManager& config = ConfigManager::getInstance();
    
    // Get WiFi config
    WiFiConfig wifiConfig = config.getWiFiConfig();
    
    // Connect to WiFi if SSID is not empty
    if (!wifiConfig.ssid.isEmpty()) {
        Serial.printf("Connecting to WiFi: %s\n", wifiConfig.ssid.c_str());
        WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
        
        // Wait for connection (with timeout)
        uint8_t timeout_counter = 0;
        while (WiFi.status() != WL_CONNECTED && timeout_counter < 20) { // 10 second timeout
            delay(500);
            Serial.print(".");
            timeout_counter++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.println("WiFi connected");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println();
            Serial.println("WiFi connection failed. Continuing without WiFi.");
        }
    } else {
        Serial.println("WiFi SSID not configured. Not connecting.");
    }
}

void time_init() {
    // Get ConfigManager instance
    ConfigManager& config = ConfigManager::getInstance();
    
    // Get NTP config
    NTPConfig ntpConfig = config.getNTPConfig();
    const char* ntpServer = ntpConfig.server.c_str();
    const char* timezone = ntpConfig.timezone.c_str();
    
    // Configure time
    configTzTime(timezone, ntpServer);
    
    // Wait for time to be set
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(100);
        now = time(nullptr);
    }
    
    // Set timezone
    setenv("TZ", timezone, 1);
    tzset();
    
    Serial.println("Time synchronized");
}

void sdcard_init() {
    // Get ConfigManager instance
    ConfigManager& config = ConfigManager::getInstance();
    
    // Initialize SPI with the correct pins for ESP32-S3
    SPI.begin(12, 11, 13, SD_CS);  // SCK, MISO, MOSI, SS
    
    Serial.println("[DEBUG] Initializing SD card using SPI mode...");
    Serial.printf("[DEBUG] Using CS pin: %d\n", SD_CS);
    
    // Try to initialize SD card
    if (!SD.begin(SD_CS)) {
        Serial.println("[ERROR] SD Card Mount Failed!");
        Serial.println("Please check:");
        Serial.println("1. Is the SD card properly inserted?");
        Serial.println("2. Is the SD card formatted as FAT32?");
        Serial.println("3. Are the SD card pins correctly connected?");
        Serial.println("4. Is the SD card slot working?");
        config.setSDCardPresent(false);
        return;
    }
    
    // If we get here, SD card is initialized
    uint8_t cardType = SD.cardType();
    
    if (cardType == CARD_NONE) {
        Serial.println("[WARN] No SD card detected or card not properly connected");
        config.setSDCardPresent(false);
        return;
    }
    
    // Print card type
    Serial.print("[DEBUG] SD Card Type: ");
    switch (cardType) {
        case CARD_MMC:    Serial.println("MMC"); break;
        case CARD_SD:     Serial.println("SDSC"); break;
        case CARD_SDHC:   Serial.println("SDHC"); break;
        case CARD_UNKNOWN:Serial.println("UNKNOWN"); break;
        default:          Serial.println("No card present"); break;
    }
    
    if (cardType != CARD_NONE) {
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("[DEBUG] SD Card Size: %lluMB\n", cardSize);
        Serial.printf("[DEBUG] Total space: %llu bytes\n", SD.totalBytes());
        Serial.printf("[DEBUG] Used space: %llu bytes\n", SD.usedBytes());
        
        // List files in root directory for debugging
        File root = SD.open("/");
        if (root) {
            Serial.println("[DEBUG] Root directory contents:");
            File file = root.openNextFile();
            while (file) {
                if (!file.isDirectory()) {
                    Serial.print("  FILE: ");
                    Serial.print(file.name());
                    Serial.print("\tSIZE: ");
                    Serial.println(file.size());
                }
                file = root.openNextFile();
            }
            root.close();
        }
        
        // Update config with actual card info
        config.setSDCardPresent(true);
        config.setSDCardSize(cardSize);
    } else {
        config.setSDCardPresent(false);
    }
}

void sensors_init() {
    // Get ConfigManager instance
    ConfigManager& config = ConfigManager::getInstance();
    
    // Get I2C pins from config
    int sdaPin = config.getI2CSDAPin();
    int sclPin = config.getI2CSCLPin();
    
    // Initialize I2C
    Wire.begin(sdaPin, sclPin);
    
    // Check if SGP30 is enabled in config
    if (config.isSGP30Enabled()) {
        if (!sgp.begin()) {
            Serial.println("SGP30 sensor not found");
            config.setSGP30Available(false);
        } else {
            Serial.println("SGP30 sensor initialized");
            config.setSGP30Available(true);
        }
    } else {
        Serial.println("SGP30 sensor disabled in config");
        config.setSGP30Available(false);
    }
    
    // Check if SHT31 is enabled in config
    if (config.isSHT31Enabled()) {
        uint8_t sht31Address = config.getSHT31I2CAddress();
        if (!sht31.begin(sht31Address)) {
            Serial.println("SHT31 sensor not found");
            config.setSHT31Available(false);
        } else {
            Serial.println("SHT31 sensor initialized");
            config.setSHT31Available(true);
            
            // Configure SHT31 settings from config
            sht31.heater(config.isSHT31HeaterEnabled());
            
            // Set SHT31 resolution if supported
            #ifdef SHT31_DISPLAY_RESOLUTION
            sht31.setResolution(SHT31_MS);
            #endif
        }
    } else {
        Serial.println("SHT31 sensor disabled in config");
        config.setSHT31Available(false);
    }
    
    // If both sensors are available, set SGP30 environmental data
    if (config.isSHT31Available() && config.isSGP30Available()) {
        // Read initial temperature and humidity to set SGP30 environmental data
        float temperature = sht31.readTemperature();
        float humidity = sht31.readHumidity();
        
        if (!isnan(temperature) && !isnan(humidity)) {
            // Convert to fixed point 8.8 format (in g/m^3)
            uint16_t absoluteHumidity = (uint16_t)(1000.0 * 256.0 * (6.112 * exp((17.67 * temperature) / (temperature + 243.5)) * humidity * 2.1674) / (273.15 + temperature));
            sgp.setHumidity(absoluteHumidity);
            Serial.println("SGP30 environmental data set from SHT31");
        }
    }
}

void audio_init() {
    // Initialize the audio manager
    AudioManager::getInstance().begin();
    AudioManager::getInstance().setVolume(50);
    Serial.println("Audio initialized");
}

void web_server_init() {
    // Get ConfigManager instance
    ConfigManager& config = ConfigManager::getInstance();
    
    // Handle root URL
    server.on("/", HTTP_GET, []() {
        if (SPIFFS.exists("/www/index.html")) {
            server.sendHeader("Location", "/index.html", true);
            server.send(302, "text/plain", "");
        } else {
            server.send(200, "text/plain", "Web Radio Alarm Clock - Please upload the web interface files to SPIFFS");
        }
    });
    
    // Serve static files from SPIFFS
    server.serveStatic("/index.html", SPIFFS, "/www/index.html");
    server.serveStatic("/css", SPIFFS, "/www/css");
    server.serveStatic("/js", SPIFFS, "/www/js");
    server.serveStatic("/img", SPIFFS, "/www/img");
    
    // Handle 404
    server.onNotFound([]() {
        server.send(404, "text/plain", "Not found");
    });
    
    // Initialize ElegantOTA
    ElegantOTA.begin(&server);
    
    // Configure ElegantOTA with credentials from config
    ElegantOTA.setAuth(
        config.getOTAUri().c_str(),
        config.getOTAPassword().c_str()
    );
    
    // Start server
    server.begin();
    
    Serial.println("Web server started with ElegantOTA");
}

void update_display_task(void *parameter) {
    // Get the UIManager instance
    UIManager& ui = UIManager::getInstance();
    DisplayManager& display = DisplayManager::getInstance();
    
    // Variables for time tracking
    unsigned long previousTimeUpdate = 0;
    String lastDisplayedTime = "";
    
    // Variables for touch handling
    bool lastTouched = false;
    uint32_t touchStartTime = 0;
    
    // Variables for WiFi status updates
    uint32_t lastWifiStatusUpdate = 0;
    const uint32_t WIFI_STATUS_UPDATE_INTERVAL = 30000; // Update every 30 seconds to reduce flicker
    
    // German weekday names
    String weekdays_de[] = {"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"};
    
    // Create a structure to store current time components
    struct tm timeinfo;
    
    // Screen state tracking
    static bool onHomeScreen = true;

    // Task loop
    while (1) {
        // Get current time
        uint32_t currentTime = millis();
        
        // Process LVGL tasks via DisplayManager
        display.update(); // Process LVGL tasks + touch events
        
        // Update time display every second
        if (currentTime - previousTimeUpdate >= 1000) {
            previousTimeUpdate = currentTime;
            
            // Get local time
            time_t now;
            time(&now);
            localtime_r(&now, &timeinfo);
            
            // Format time string
            char timeString[10];
            sprintf(timeString, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            
            // Only update if time changed
            if (String(timeString) != lastDisplayedTime) {
                lastDisplayedTime = timeString;
                Serial.printf("[DEBUG] Updating time from '%s' to '%s'\n", lastDisplayedTime.c_str(), timeString);
                
                // Update time on display
                ui.updateTime(timeString);
                
                // Update date once per minute or on initial startup
                static bool firstUpdate = true;
                if (timeinfo.tm_sec == 0 || firstUpdate) {
                    char dateString[32];
                    
                    // Simpler, more reliable date format: "Weekday, dd.mm.yyyy"
                    sprintf(dateString, "%s, %02d.%02d.%04d", 
                            weekdays_de[timeinfo.tm_wday].c_str(), 
                            timeinfo.tm_mday, 
                            timeinfo.tm_mon + 1, 
                            timeinfo.tm_year + 1900);
                    
                    Serial.printf("Setting date: %s\n", dateString);
                    ui.updateDate(dateString);
                    firstUpdate = false;
                }
            }
        }
        
        // Update WiFi status information periodically
        if (currentTime - lastWifiStatusUpdate >= WIFI_STATUS_UPDATE_INTERVAL) {
            // Update WiFi SSID
            if (WiFi.status() == WL_CONNECTED) {
                ui.updateWifiSsid(WiFi.SSID().c_str());
                
                // Update IP address
                char ipAddress[16];
                snprintf(ipAddress, sizeof(ipAddress), "%d.%d.%d.%d", 
                    WiFi.localIP()[0], WiFi.localIP()[1], 
                    WiFi.localIP()[2], WiFi.localIP()[3]);
                ui.updateIpAddress(ipAddress);
                
                // Update WiFi signal quality (RSSI to percentage)
                int rssi = WiFi.RSSI();
                int quality = 0;
                
                if (rssi <= -100) {
                    quality = 0;
                } else if (rssi >= -50) {
                    quality = 100;
                } else {
                    // Convert RSSI to percentage (RSSI range: -100 to -50)
                    quality = 2 * (rssi + 100);
                }
                
                ui.updateWifiQuality(quality);
            } else {
                // Not connected
                ui.updateWifiSsid("Not Connected");
                ui.updateIpAddress("---");
                ui.updateWifiQuality(-1); // Negative value indicates not connected
            }
            
            lastWifiStatusUpdate = currentTime;
        }
        
        // Small delay to allow other tasks to run
        vTaskDelay(pdMS_TO_TICKS(1)); // 1ms delay for high refresh rate
    }
    vTaskDelete(NULL);
}
void update_sensors_task(void *parameter) {
    // Get ConfigManager and UIManager instances once
    ConfigManager& config = ConfigManager::getInstance();
    UIManager& ui = UIManager::getInstance();
    
    // Timing variables
    uint32_t lastSHT31Read = 0;
    uint32_t lastSGP30Read = 0;
    const uint32_t SHT31_READ_INTERVAL = 10000;  // 10 seconds
    const uint32_t SGP30_READ_INTERVAL = 5000;  // 5 seconds
    
    while (1) {
        uint32_t currentTime = millis();
        
        // Read SHT31 sensor (temperature and humidity) if enabled and time has passed
        if (config.isSHT31Available() && (currentTime - lastSHT31Read >= SHT31_READ_INTERVAL)) {
            float temperature = sht31.readTemperature();
            float humidity = sht31.readHumidity();
            
            if (!isnan(temperature) && !isnan(humidity)) {
                // Update UI with new values
                ui.updateTemperature(temperature);
                ui.updateHumidity(humidity);
                
                // If SGP30 is available, update its environmental data
                if (config.isSGP30Available()) {
                    // Convert to fixed point 8.8 format (in g/m^3)
                    uint16_t absoluteHumidity = (uint16_t)(1000.0 * 256.0 * (6.112 * exp((17.67 * temperature) / (temperature + 243.5)) * humidity * 2.1674) / (273.15 + temperature));
                    sgp.setHumidity(absoluteHumidity);
                }
                
                // Debug output
                Serial.printf("Temperature: %.1f°C, Humidity: %.1f%%\n", temperature, humidity);
            }
            
            lastSHT31Read = currentTime;
        }
        
        // Read SGP30 sensor (TVOC and eCO2) if enabled and time has passed
        if (config.isSGP30Available() && (currentTime - lastSGP30Read >= SGP30_READ_INTERVAL)) {
            if (sgp.IAQmeasure()) {
                uint16_t tvoc = sgp.TVOC;
                uint16_t eco2 = sgp.eCO2;
                
                // Update UI with new values
                ui.updateTVOC(tvoc);
                ui.updateCO2(eco2);
                
                // Debug output
                Serial.printf("TVOC: %d ppb, eCO2: %d ppm\n", tvoc, eco2);
            }
            
            lastSGP30Read = currentTime;
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void check_alarms_task(void *parameter) {
    while (1) {
        // Check if any alarms need to be triggered
        AlarmManager::getInstance().checkAlarms();
        
        // Sleep for 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void update_weather_task(void *parameter) {
    // Get the WeatherService instance
    WeatherService& weatherService = WeatherService::getInstance();
    UIManager& ui = UIManager::getInstance();
    
    // Allow time for WiFi to connect before initializing
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    // Initialize weather service
    if (!weatherService.init()) {
        Serial.println("[ERROR] Failed to initialize WeatherService. Weather data will not be available.");
        vTaskDelete(NULL);
        return;
    }
    
    // Force initial update
    bool initialUpdateSuccess = weatherService.forceUpdate();
    if (!initialUpdateSuccess) {
        Serial.println("[WARNING] Initial weather update failed. Will retry later.");
    } else {
        // Update UI with current weather data
        const WeatherService::CurrentWeather& current = weatherService.getCurrentWeather();
        const WeatherService::DailyForecast& today = weatherService.getDailyForecast(0);
        
        // Update current weather in UI
        ui.updateCurrentWeather(
            current.temp,
            current.feels_like,
            current.weather_description.c_str(),
            current.weather_icon.c_str()
        );
        
        // Update morning forecast (using morning temp from today's forecast)
        ui.updateMorningForecast(
            today.temp.morn,
            today.pop,
            today.weather_icon.c_str()
        );
        
        // Update afternoon forecast (using day temp from today's forecast)
        ui.updateAfternoonForecast(
            today.temp.day,
            today.pop,
            today.weather_icon.c_str()
        );
    }
    
    while (1) {
        // Update weather data
        bool updated = weatherService.update();
        
        if (updated) {
            // Update UI with new weather data
            const WeatherService::CurrentWeather& current = weatherService.getCurrentWeather();
            const WeatherService::DailyForecast& today = weatherService.getDailyForecast(0);
            
            // Update current weather in UI
            ui.updateCurrentWeather(
                current.temp,
                current.feels_like,
                current.weather_description.c_str(),
                current.weather_icon.c_str()
            );
            
            // Update morning forecast (using morning temp from today's forecast)
            ui.updateMorningForecast(
                today.temp.morn,
                today.pop,
                today.weather_icon.c_str()
            );
            
            // Update afternoon forecast (using day temp from today's forecast)
            ui.updateAfternoonForecast(
                today.temp.day,
                today.pop,
                today.weather_icon.c_str()
            );
            
            Serial.println("[INFO] Weather UI updated successfully");
        }
        
        // Sleep for 5 minutes (300,000 ms)
        // The WeatherService class will handle throttling of API calls
        vTaskDelay(pdMS_TO_TICKS(300000));
    }
}
