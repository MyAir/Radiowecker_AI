#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>
#include <SPIFFS.h>
#include <time.h>
#include <lvgl.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Update.h>
#include <driver/i2s.h>
#include <TFT_eSPI.h>

// Forward declarations
class AudioGenerator;
class AudioOutputI2S;
class AudioFileSourceBuffer;
class AudioFileSourceICYStream;
class AudioFileSourceSD;
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorMP3.h>
#include <Adafruit_SGP30.h>
#include <Adafruit_SHT31.h>
#include <Wire.h>
#include "ConfigManager.h"
#include "UIManager.h"
#include "AudioManager.h"
#include "AlarmManager.h"

// Display configuration
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40,  // DE
    41,  // VSYNC
    39,  // HSYNC
    42,  // PCLK
    45,  // R0
    48,  // R1
    47,  // R2
    21,  // R3
    14,  // R4
    5,   // G0
    6,   // G1
    7,   // G2
    15,  // G3
    16,  // G4
    4,   // G5
    8,   // B0
    3,   // B1
    46,  // B2
    9,   // B3
    1,   // B4
    0,   // hsync_polarity
    8,   // hsync_front_porch
    4,   // hsync_pulse_width
    8,   // hsync_back_porch
    0,   // vsync_polarity
    8,   // vsync_front_porch
    4,   // vsync_pulse_width
    8,   // vsync_back_porch
    1,   // pclk_active_neg
    16000000  // prefer_speed
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    800,  // width
    480,  // height
    rgbpanel,
    0,    // rotation
    true  // auto_flush
);

// Touch configuration
#define TOUCH_GT911_SCL 18
#define TOUCH_GT911_SDA 17
#define TOUCH_GT911_INT 16
#define TOUCH_GT911_RST 15
#define TOUCH_GT911_ROTATION ROTATION_INVERTED

// Backlight control
#define BACKLIGHT_PIN 44

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
Adafruit_SGP30 sgp;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Web server instance
AsyncWebServer server(80);

// Global instances
ConfigManager& configManager = ConfigManager::getInstance();
UIManager& uiManager = UIManager::getInstance();
AudioManager& audioManager = AudioManager::getInstance();
AlarmManager& alarmManager = AlarmManager::getInstance();

// Task handles
TaskHandle_t lvglTaskHandle = nullptr;
TaskHandle_t mainTaskHandle = nullptr;
TaskHandle_t audioTaskHandle = nullptr;

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
    uiManager.showAlarmScreen(alarm);
    
    // Play alarm sound
    if (alarm.source == 0) { // Radio
        // Get radio station URL from config
        const char* streamUrl = configManager.getRadioStationUrl(alarm.sourceData.stationIndex);
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
    Serial.begin(115200);
    
    // Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An error occurred while mounting SPIFFS");
    } else {
        Serial.println("SPIFFS mounted successfully");
    }
    
    // Initialize display
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    
    // Initialize LVGL
    lvgl_init();
    
    // Initialize SD card first (needed for config and audio files)
    sdcard_init();
    
    // Load configuration
    if (!configManager.loadConfig()) {
        Serial.println("Using default configuration");
    }
    
    // Initialize WiFi
    wifi_init();
    
    // Initialize time
    time_init();
    
    // Initialize audio
    audioManager.begin();
    
    // Initialize alarms
    alarmManager.setAlarmTriggerCallback(onAlarmTriggered);
    alarmManager.begin();
    
    // Initialize sensors
    sensors_init();
    
    // Initialize web server
    web_server_init();
    
    // Create tasks
    xTaskCreatePinnedToCore(
        lvgl_tick,       // Task function
        "LVGL",          // Task name
        4096,            // Stack size (bytes)
        NULL,            // Parameter
        1,               // Priority
        &lvglTaskHandle, // Task handle
        1                // Core (1 for LVGL)
    );
    
    xTaskCreatePinnedToCore(
        update_display_task,
        "Display",
        8192,
        NULL,
        1,
        &mainTaskHandle,
        0
    );
    
    xTaskCreatePinnedToCore(
        update_sensors_task,
        "Sensors",
        4096,
        NULL,
        1,
        NULL,
        0
    );
    
    xTaskCreatePinnedToCore(
        [] (void* pvParameters) {
            audioManager.loop();
        },
        "Audio",
        4096,
        NULL,
        2,  // Higher priority for audio
        &audioTaskHandle,
        0
    );
    
    // Delete setup/loop task
    vTaskDelete(NULL);
}

void loop() {
    // This should never be reached as we delete the loop task in setup()
    vTaskDelete(NULL);
}

void lvgl_init() {
  // Initialize LVGL
  lv_init();
  
  // Initialize display buffer
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 800 * 10);
  
  // Initialize TFT
  tft.begin();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(TFT_BLACK);
  
  // Initialize display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 800;
  disp_drv.ver_res = 480;
  disp_drv.flush_cb = [](lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    
    lv_disp_flush_ready(disp);
  };
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  
  // Initialize touch (GT911)
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = [](lv_indev_drv_t *drv, lv_indev_data_t *data) {
    static int16_t last_x = 0;
    static int16_t last_y = 0;
    uint8_t touched = 0;
    
    // TODO: Implement GT911 touch reading
    // For now, simulate no touch
    data->point.x = last_x;
    data->point.y = last_y;
    data->state = LV_INDEV_STATE_REL;
    
    return false;
  };
  lv_indev_drv_register(&indev_drv);
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
  
  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);
  
  Serial.println("HTTP server started");
}

void lvgl_tick(void *parameter) {
  while (1) {
    lv_tick_inc(5);
    delay(5);
  }
}

void update_display_task(void *parameter) {
    while (1) {
        // Handle LVGL tasks
        lv_task_handler();
        
        // Update time display
        static unsigned long lastTimeUpdate = 0;
        if (millis() - lastTimeUpdate >= 1000) {  // Update every second
            lastTimeUpdate = millis();
            
            // Get current time
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                // Update time on display
                char timeStr[9];
                strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
                uiManager.updateTime(timeStr);
                
                // Update date on display
                char dateStr[32];
                strftime(dateStr, sizeof(dateStr), "%A, %B %d", &timeinfo);
                uiManager.updateDate(dateStr);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(5));  // ~200 FPS for LVGL
    }
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
    while (1) {
        // Update alarm manager
        alarmManager.update();
        
        // Check for next alarm
        Alarm nextAlarm = alarmManager.getNextAlarm();
        if (nextAlarm.id != 0) {
            // Update UI with next alarm time
            char alarmStr[32];
            snprintf(alarmStr, sizeof(alarmStr), "Alarm: %02d:%02d", 
                    nextAlarm.hour, nextAlarm.minute);
            uiManager.updateNextAlarm(alarmStr);
        } else {
            uiManager.updateNextAlarm("No alarms set");
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
