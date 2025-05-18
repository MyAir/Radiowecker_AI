#pragma once

#include <ArduinoJson.h>
#include <SD_MMC.h>
#include <SPIFFS.h>
#include <vector>

#define CONFIG_FILE "/sdcard/config.json"
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
    String api_key;
    String city_id;
    String units;
    uint16_t update_interval;
};

struct SystemConfig {
    String hostname;
    String ota_password;
};

class ConfigManager {
public:
    bool begin();
    bool loadConfig();
    bool saveConfig();
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
    
    // Setters
    void setWiFiConfig(const WiFiConfig& config) { wifiConfig = config; }
    void setNTPConfig(const NTPConfig& config) { ntpConfig = config; }
    void setDisplayConfig(const DisplayConfig& config) { displayConfig = config; }
    void setAlarms(const std::vector<AlarmConfig>& alarmList) { alarms = alarmList; }
    void setRadioStations(const std::vector<RadioStation>& stations) { radioStations = stations; }
    void setWeatherConfig(const WeatherConfig& config) { weatherConfig = config; }
    void setSystemConfig(const SystemConfig& config) { systemConfig = config; }
    void setFallbackAudio(const String& path) { fallbackAudio = path; }
    
private:
    WiFiConfig wifiConfig;
    NTPConfig ntpConfig;
    DisplayConfig displayConfig;
    std::vector<AlarmConfig> alarms;
    std::vector<RadioStation> radioStations;
    WeatherConfig weatherConfig;
    SystemConfig systemConfig;
    String fallbackAudio;
    
    bool parseConfig(JsonDocument& doc);
    void setDefaultConfig();
};

extern ConfigManager configManager;
