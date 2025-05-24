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

// Forward declarations for manager classes
#include "DisplayManager.h"
#include "UIManager.h"
#include "ConfigManager.h"
#include "AudioManager.h"
#include "AlarmManager.h"

// Task handles
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t lvglTaskHandle = nullptr;
TaskHandle_t sensorsTaskHandle = NULL;
TaskHandle_t alarmTaskHandle = NULL;

Adafruit_SGP30 sgp;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Web server
WebServer server(80);

// LVGL display buffer is now managed by DisplayManager

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
    
    // Check if all tasks were created successfully
    if (displayTaskCreated != pdPASS || 
        sensorsTaskCreated != pdPASS || 
        alarmsTaskCreated != pdPASS) {
        
        Serial.println("Error: Failed to create one or more tasks!");
        while (1) { delay(1000); } // Halt if tasks can't be created
    }
    
    Serial.println("Setup complete - System is running");
    Serial.println("------------------------------------");
    
    // Show home screen using the singleton instance
    ui.showHomeScreen();
    
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
    if (millis() - lastTimeUpdate >= 1000) {
        lastTimeUpdate = millis();
        
        // Get current time
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        
        // Format time as HH:MM
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
        
        // Check if minute has changed
        static int lastMinute = -1;
        bool minuteChanged = (timeinfo.tm_min != lastMinute);
        
        if (minuteChanged) {
            lastMinute = timeinfo.tm_min;
            
            // Format date (e.g., "Monday, January 01")
            char dateStr[32];
            strftime(dateStr, sizeof(dateStr), "%A, %B %d", &timeinfo);
            
            // Update UI with date/time
            ui.updateTime(timeStr);
        } else {
            // Update just the time
            ui.updateTime(timeStr);
        }
        
        // Debug output
        Serial.printf("Time updated: %s\n", timeStr);
    }
    
    // Small delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(10));
}

void wifi_init() {
    // Get ConfigManager instance
    ConfigManager& config = ConfigManager::getInstance();
    
    // Get WiFi credentials from config
    WiFiConfig wifiConfig = config.getWiFiConfig();
    const char* ssid = wifiConfig.ssid.c_str();
    const char* password = wifiConfig.password.c_str();
    
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
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
    
    // SD card configuration - use hardcoded values since ConfigManager doesn't have these methods
    const char* mountPoint = "/sdcard";
    bool mode1bit = true; // 1-bit mode
    
    // Initialize SD card
    if (!SD_MMC.begin(mountPoint, mode1bit)) {
        Serial.println("SD Card Mount Failed");
        return;
    }
    
    uint8_t cardType = SD_MMC.cardType();
    
    if (cardType == CARD_NONE) {
        Serial.println("No SD card attached");
        return;
    }
    
    Serial.print("SD Card Type: ");
    
    if (cardType == CARD_MMC) {
        Serial.println("MMC");
    } else if (cardType == CARD_SD) {
        Serial.println("SDSC");
    } else if (cardType == CARD_SDHC) {
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }
    
    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
    // Update config with actual card info
    config.setSDCardPresent(true);
    config.setSDCardSize(cardSize);
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
    while (1) {
        // Update the display using the singleton instance
        DisplayManager::getInstance().update();
        
        // Sleep for 50ms (20 FPS)
        vTaskDelay(pdMS_TO_TICKS(50));
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
    const uint32_t SHT31_READ_INTERVAL = 2000;  // 2 seconds
    const uint32_t SGP30_READ_INTERVAL = 1000;  // 1 second
    
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
