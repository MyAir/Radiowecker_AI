#pragma once

#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <vector>

// SD Card CS pin - defined in platformio.ini

#define CONFIG_FILE "/config.json"
#define CONFIG_TEMPLATE "/config.json"

struct WiFiConfig {
    String ssid;
    String password;
};

struct NTPConfig {
    String server;
    String timezone;
};

struct DisplayConfig {
    uint8_t brightness;
    uint8_t timeout;
    bool auto_brightness;
    String theme;
};

struct AlarmConfig {
    uint8_t id;
    bool enabled;
    uint8_t hour;
    uint8_t minute;
    bool days[7];  // 0=Sunday, 6=Saturday
    String type;    // "radio" or "tone"
    uint8_t station_id;
    uint8_t volume;
    uint8_t fade_in;  // seconds
    uint16_t duration; // minutes
};

struct RadioStation {
    uint8_t id;
    String name;
    String url;
    String genre;
};

struct WeatherConfig {
    String appid;       // API key (renamed from api_key to match OpenWeatherMap)
    float lat;          // Latitude
    float lon;          // Longitude
    String units;       // Units (metric, imperial)
    String lang;        // Language code
    uint16_t update_interval; // Update interval in minutes
};

struct SystemConfig {
    String hostname;
    String ota_password;
};

class ConfigManager {
private:
    static ConfigManager* instance;  // Declaration only
    
    // Private constructor
    ConfigManager() = default;
    
    // Prevent copying and assignment
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    WiFiConfig wifiConfig;
    NTPConfig ntpConfig;
    DisplayConfig displayConfig;
    std::vector<AlarmConfig> alarms;
    std::vector<RadioStation> radioStations;
    WeatherConfig weatherConfig;
    SystemConfig systemConfig;
    String fallbackAudio;
    
    // Sensor states and configuration
    bool sdCardPresent = false;
    uint64_t sdCardSize = 0;
    bool sht31Available = false;
    bool sgp30Available = false;
    int i2cSDAPin = 38; // Default values
    int i2cSCLPin = 37;
    uint8_t sht31Address = 0x44;
    bool sht31HeaterEnabled = false;
    bool sht31Enabled = true;
    bool sgp30Enabled = true;
    
    bool parseConfig(JsonDocument& doc);
    void setDefaultConfig();
    
public:
    // Copy constructor and assignment operator already deleted above
    
    // Static method to get the singleton instance
    static ConfigManager& getInstance() {
        if (!instance) {
            instance = new ConfigManager();
        }
        return *instance;
    }
    
    bool begin();
    bool loadConfig();
    bool loadConfigFromSPIFFS();
    bool saveConfig();
    bool saveConfigToSPIFFS();
    bool resetToDefault();
    
    // Getters
    WiFiConfig getWiFiConfig() { return wifiConfig; }
    NTPConfig getNTPConfig() { return ntpConfig; }
    DisplayConfig getDisplayConfig() { return displayConfig; }
    std::vector<AlarmConfig> getAlarms() { return alarms; }
    std::vector<RadioStation> getRadioStations() { return radioStations; }
    WeatherConfig getWeatherConfig() { return weatherConfig; }
    SystemConfig getSystemConfig() { return systemConfig; }
    String getFallbackAudio() { return fallbackAudio; }
    
    // OTA settings
    String getOTAUri() { return systemConfig.hostname; }
    String getOTAPassword() { return systemConfig.ota_password; }
    
    // Sensor configuration getters
    int getI2CSDAPin() { return i2cSDAPin; }
    int getI2CSCLPin() { return i2cSCLPin; }
    bool isSHT31Enabled() { return sht31Enabled; }
    bool isSGP30Enabled() { return sgp30Enabled; }
    uint8_t getSHT31I2CAddress() { return sht31Address; }
    bool isSHT31HeaterEnabled() { return sht31HeaterEnabled; }
    bool isSHT31Available() { return sht31Available; }
    bool isSGP30Available() { return sgp30Available; }
    bool isSDCardPresent() { return sdCardPresent; }
    
    // Setters
    void setWiFiConfig(const WiFiConfig& config) { wifiConfig = config; }
    void setNTPConfig(const NTPConfig& config) { ntpConfig = config; }
    void setDisplayConfig(const DisplayConfig& config) { displayConfig = config; }
    void setAlarms(const std::vector<AlarmConfig>& alarmList) { alarms = alarmList; }
    void setRadioStations(const std::vector<RadioStation>& stations) { radioStations = stations; }
    void setWeatherConfig(const WeatherConfig& config) { weatherConfig = config; }
    void setSystemConfig(const SystemConfig& config) { systemConfig = config; }
    void setFallbackAudio(const String& path) { fallbackAudio = path; }
    
    // Sensor configuration setters
    void setI2CPins(int sda, int scl) { i2cSDAPin = sda; i2cSCLPin = scl; }
    void setSHT31Enabled(bool enabled) { sht31Enabled = enabled; }
    void setSGP30Enabled(bool enabled) { sgp30Enabled = enabled; }
    void setSHT31Address(uint8_t address) { sht31Address = address; }
    void setSHT31HeaterEnabled(bool enabled) { sht31HeaterEnabled = enabled; }
    void setSHT31Available(bool available) { sht31Available = available; }
    void setSGP30Available(bool available) { sgp30Available = available; }
    void setSDCardPresent(bool present) { sdCardPresent = present; }
    void setSDCardSize(uint64_t size) { sdCardSize = size; }
};

