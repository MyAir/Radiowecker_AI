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
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

// Hardware-specific libraries
#include <Arduino_GFX_Library.h>
#include <Adafruit_FT6206.h>

// LVGL and display helpers
#include "lvgl_helper.h"

// Local includes
#include "Audio.h"  // Using local Audio.h from ESP32-audioI2S

// Audio components
#include "AudioGenerator.h"
#include "AudioOutputI2S.h"
class AudioFileSourceBuffer;
class AudioFileSourceICYStream;
class AudioFileSourceSD;
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_SHT31.h>
#include "DisplayManager.h"
#include "ConfigManager.h"
#include "UIManager.h"
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
#include "Audio.h"
Audio audio;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
Preferences preferences;

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

// Global display instance
Arduino_DataBus *bus = new Arduino_ESP32RGBPanel(
    40,   // DE
    41,   // VSYNC
    39,   // HSYNC
    42,   // PCLK
    45,   // R0
    48,   // R1
    47,   // R2
    21,   // R3
    14,   // R4
    5,    // G0
    6,    // G1
    7,    // G2
    15,   // G3
    16,   // G4
    4,    // G5
    8,    // B0
    3,    // B1
    46,   // B2
    9,    // B3
    1,    // B4
    0,    // hsync_polarity
    8,    // hsync_front_porch
    4,    // hsync_pulse_width
    8,    // hsync_back_porch
    0,    // vsync_polarity
    8,    // vsync_front_porch
    4,    // vsync_pulse_width
    8,    // vsync_back_porch
    1,    // pclk_active_neg
    16000000  // prefer_speed
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    800,    // width
    480,    // height
    bus,
    0,      // RST
    0,      // rotation
    true,   // auto_flush
    bus
);

// Global instances
UIManager& uiManager = UIManager::getInstance();
ConfigManager configManager;
AudioManager& audioManager = AudioManager::getInstance();
AlarmManager& alarmManager = AlarmManager::getInstance();

// Task handles
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
TaskHandle_t lvglTaskHandle = nullptr;
TaskHandle_t sensorsTaskHandle = NULL;
TaskHandle_t alarmTaskHandle = NULL;

Adafruit_SGP30 sgp;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
AsyncWebServer server(80);

// LVGL display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[800 * 10];
static lv_color_t buf2[800 * 10];

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
    // Update UI to show alarm is ringing
    uiManager.showAlarmScreen();
    
    // Play alarm sound
    if (alarm.source == 0) { // Radio
        // Get radio station URL from config
        // Get the radio station URL from the config manager
        const char* streamUrl = "";
        if (alarm.sourceData.stationIndex < configManager.getRadioStations().size()) {
            streamUrl = configManager.getRadioStations()[alarm.sourceData.stationIndex].url.c_str();
        }
        if (streamUrl) {
            audioManager.playStream(streamUrl);
        }
    } else { // MP3
        audioManager.playFile(alarm.sourceData.filepath);
    }
    
    // Set volume
    audioManager.setVolume(alarm.volume);
}

void setup() {
    // Initialize serial
    Serial.begin(115200);
    while (!Serial);
    
    // Initialize display
    if (gfx->begin()) {
        gfx->fillScreen(BLACK);
        gfx->setTextSize(2);
        gfx->setTextColor(WHITE, BLACK);
        gfx->setCursor(10, 10);
        gfx->println("Initializing display...");
        Serial.println("Display initialized");
    } else {
        Serial.println("Display initialization failed!");
        while (1);
    }

    // Initialize LVGL with the display
    lvgl_init(gfx, 17, 18, -1, 0x14); // SDA, SCL, RST, I2C address
    
    // Initialize file system
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
        while (1);
    } else {
        Serial.println("SPIFFS mounted successfully");
    }
    
    // Initialize WiFi
    wifi_init();

    // Initialize time
    time_init();
    
    // Initialize SD card
    sdcard_init();
    
    // Initialize sensors
    sensors_init();
    
    // Initialize audio
    audio_init();
    
    // Initialize web server
    web_server_init();

    // Initialize UI
    uiManager.init();

    // Create tasks
    xTaskCreatePinnedToCore(update_display_task, "Display", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(update_sensors_task, "Sensors", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(check_alarms_task, "Alarms", 4096, NULL, 1, NULL, 1);

    // Set alarm callback
    alarmManager.setAlarmCallback(onAlarmTriggered);

    // Show main screen
    uiManager.showMainScreen();

    Serial.println("Setup complete");
}

void loop() {
    // Handle OTA updates
    ArduinoOTA.handle();
    
    // Handle audio
    audio.loop();
    
    // Update time every second
    static unsigned long lastTimeUpdate = 0;
    if (millis() - lastTimeUpdate >= 1000) {
        lastTimeUpdate = millis();
        
        // Update time string
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
        
        // Update date string once a minute
        static int lastMinute = -1;
        if (timeinfo.tm_min != lastMinute) {
            lastMinute = timeinfo.tm_min;
            
            char dateStr[32];
            strftime(dateStr, sizeof(dateStr), "%A, %d %B %Y", &timeinfo);
            // uiManager.updateDate(dateStr); // Update UI if needed
        }
    }
    
    // Small delay to prevent watchdog issues
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

void wifi_init() {
    // TODO: Load WiFi credentials from config
    const char* ssid = "your_ssid";
    const char* password = "your_password";
    
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void time_init() {
    configTime(0, 0, "pool.ntp.org");
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void sdcard_init() {
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("SD Card Mount Failed");
        return;
    }
    
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
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
}

void sensors_init() {
    Wire.begin(I2C_SDA, I2C_SCL);
    
    if (!sgp.begin()) {
        Serial.println("SGP30 sensor not found");
    }
    
    if (!sht31.begin(0x44)) {
        Serial.println("SHT31 sensor not found");
    }
}

void audio_init() {
    // Audio initialization is handled by AudioManager's begin()
    // This function is kept for compatibility
}

void web_server_init() {
    // Serve static files from SPIFFS
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
    
    // Handle file not found
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "File Not Found");
    });
    
    // Start server
    server.begin();
    
    // Start ElegantOTA if available
    #ifdef ASYNCELEGANTOTA_H_
    AsyncElegantOTA.begin(&server);
    #endif
    
    Serial.println("HTTP server started");
}

void update_display_task(void *parameter) {
    while (1) {
        // Handle LVGL tasks
        lv_task_handler();
        
        // Small delay to prevent watchdog issues
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void update_sensors_task(void *parameter) {
    while (1) {
        // Update sensor readings
        if (sht31.begin(0x44)) {  // Check if SHT31 is connected
            float temperature = sht31.readTemperature();
            float humidity = sht31.readHumidity();
            
            if (!isnan(temperature) && !isnan(humidity)) {
                char tempStr[16], humStr[16];
                snprintf(tempStr, sizeof(tempStr), "%.1f°C", temperature);
                snprintf(humStr, sizeof(humStr), "%.1f%%", humidity);
                
                uiManager.updateTemperature(tempStr);
                uiManager.updateHumidity(humStr);
            }
        }
        
        if (sgp.begin()) {  // Check if SGP30 is connected
            if (sgp.IAQmeasure()) {
                uint16_t tvoc = sgp.TVOC;
                uint16_t eco2 = sgp.eCO2;
                
                char tvocStr[16], eco2Str[16];
                snprintf(tvocStr, sizeof(tvocStr), "%d ppb", tvoc);
                snprintf(eco2Str, sizeof(eco2Str), "%d ppm", eco2);
                
                uiManager.updateTVOC(tvocStr);
                uiManager.updateCO2(eco2Str);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Update every 5 seconds
    }
}

void check_alarms_task(void *parameter) {
    while (true) {
        // Update alarm manager
        alarmManager.update();
        
        // Check if any alarms should trigger
        // For now, we'll just show a placeholder
        // In a real implementation, you would check for active alarms here
        bool hasActiveAlarm = false;
        
        // TODO: Implement actual alarm checking logic
        // For now, we'll just show a placeholder
        if (hasActiveAlarm) {
            // If we had an active alarm, we would update the UI here
            // char alarmStr[32];
            // snprintf(alarmStr, sizeof(alarmStr), "%02d:%02d", nextAlarm.hour, nextAlarm.minute);
            // uiManager.updateNextAlarm(alarmStr);
        } else {
            uiManager.updateNextAlarm("No Alarm");
        }
        
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
