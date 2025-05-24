#include "ConfigManager.h"

// Initialize static member
ConfigManager* ConfigManager::instance = nullptr;

bool ConfigManager::begin() {
    // Always try to mount SPIFFS first
    if (!SPIFFS.begin(true)) {
        Serial.println("[ERROR] Failed to mount SPIFFS");
        return false;
    }
    Serial.println("[DEBUG] SPIFFS mounted successfully");
    
    // Try to mount SD card
    bool sdcardAvailable = SD.begin(SD_CS);
    if (sdcardAvailable) {
        Serial.println("[DEBUG] SD card mounted successfully");
        
        // If SD card is available, try to use it for config
        if (SD.exists(CONFIG_FILE)) {
            Serial.println("[DEBUG] Found config file on SD card");
            return loadConfig();
        } else {
            Serial.println("[WARN] No config file found on SD card");
        }
    } else {
        Serial.println("[WARN] Failed to mount SD card, using SPIFFS only");
    }
    
    // If we get here, either SD card failed or config file doesn't exist
    // Try to use config template from SPIFFS
    if (SPIFFS.exists(CONFIG_TEMPLATE)) {
        Serial.println("[DEBUG] Using config template from SPIFFS");
        
        // If SD card is available, copy template to SD
        if (sdcardAvailable) {
            File src = SPIFFS.open(CONFIG_TEMPLATE);
            File dst = SD.open(CONFIG_FILE, FILE_WRITE);
            
            if (src && dst) {
                Serial.println("[DEBUG] Copying config template to SD card");
                while (src.available()) {
                    dst.write(src.read());
                }
                dst.close();
                src.close();
                return loadConfig();
            } else {
                Serial.println("[WARN] Failed to copy config template to SD card");
                if (!src) Serial.println("  - Failed to open source file");
                if (!dst) Serial.println("  - Failed to open destination file");
            }
        }
        
        // If SD card copy failed or not available, try to load directly from SPIFFS
        return loadConfigFromSPIFFS();
    }
    
    // No config found anywhere, create default
    Serial.println("[INFO] No config found, creating default config");
    setDefaultConfig();
    
    // Try to save to SD card first, then fall back to SPIFFS
    if (sdcardAvailable) {
        if (saveConfig()) {
            return true;
        }
    }
    
    // If we can't save to SD, save to SPIFFS
    return saveConfigToSPIFFS();
}

bool ConfigManager::loadConfigFromSPIFFS() {
    // Try to load from SPIFFS template
    File configFile = SPIFFS.open(CONFIG_TEMPLATE);
    if (!configFile) {
        Serial.println("[ERROR] Failed to open config template from SPIFFS");
        return false;
    }
    
    size_t size = configFile.size();
    if (size == 0) {
        Serial.println("[ERROR] Config template in SPIFFS is empty");
        configFile.close();
        return false;
    }
    
    // Allocate a temporary JsonDocument
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        Serial.print("[ERROR] Failed to parse config template: ");
        Serial.println(error.c_str());
        return false;
    }
    
    Serial.println("[DEBUG] Successfully loaded config from SPIFFS template");
    return parseConfig(doc);
}

bool ConfigManager::saveConfigToSPIFFS() {
    // Create a temporary file in SPIFFS
    File tempFile = SPIFFS.open("/temp_config.json", FILE_WRITE);
    if (!tempFile) {
        Serial.println("[ERROR] Failed to create temp config file in SPIFFS");
        return false;
    }
    
    // Create JSON document
    DynamicJsonDocument doc(4096);
    
    // WiFi
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = wifiConfig.ssid;
    wifi["password"] = wifiConfig.password;
    
    // NTP
    JsonObject ntp = doc.createNestedObject("ntp");
    ntp["server"] = ntpConfig.server;
    ntp["timezone"] = ntpConfig.timezone;
    
    // Display
    JsonObject display = doc.createNestedObject("display");
    display["brightness"] = displayConfig.brightness;
    display["timeout"] = displayConfig.timeout;
    display["auto_brightness"] = displayConfig.auto_brightness;
    display["theme"] = displayConfig.theme;
    
    // Alarms
    JsonArray alarmsArray = doc.createNestedArray("alarms");
    for (const auto& alarm : alarms) {
        JsonObject alarmObj = alarmsArray.createNestedObject();
        alarmObj["id"] = alarm.id;
        alarmObj["enabled"] = alarm.enabled;
        alarmObj["hour"] = alarm.hour;
        alarmObj["minute"] = alarm.minute;
        
        JsonArray daysArray = alarmObj.createNestedArray("days");
        for (bool day : alarm.days) {
            daysArray.add(day);
        }
        
        alarmObj["type"] = alarm.type;
        alarmObj["station_id"] = alarm.station_id;
        alarmObj["volume"] = alarm.volume;
        alarmObj["fade_in"] = alarm.fade_in;
        alarmObj["duration"] = alarm.duration;
    }
    
    // Radio Stations
    JsonArray stationsArray = doc.createNestedArray("radio_stations");
    for (const auto& station : radioStations) {
        JsonObject stationObj = stationsArray.createNestedObject();
        stationObj["id"] = station.id;
        stationObj["name"] = station.name;
        stationObj["url"] = station.url;
        stationObj["genre"] = station.genre;
    }
    
    // Weather
    JsonObject weather = doc.createNestedObject("weather");
    weather["api_key"] = weatherConfig.api_key;
    weather["city_id"] = weatherConfig.city_id;
    weather["units"] = weatherConfig.units;
    weather["update_interval"] = weatherConfig.update_interval;
    
    // System
    JsonObject system = doc.createNestedObject("system");
    system["hostname"] = systemConfig.hostname;
    system["ota_password"] = systemConfig.ota_password;
    
    // Fallback audio
    doc["fallback_audio"] = fallbackAudio;
    
    // Serialize JSON to file
    if (serializeJson(doc, tempFile) == 0) {
        Serial.println("[ERROR] Failed to write to temp config file in SPIFFS");
        tempFile.close();
        return false;
    }
    
    tempFile.close();
    
    // Replace old config with new one
    if (SPIFFS.exists(CONFIG_TEMPLATE)) {
        SPIFFS.remove(CONFIG_TEMPLATE);
    }
    
    if (!SPIFFS.rename("/temp_config.json", CONFIG_TEMPLATE)) {
        Serial.println("[ERROR] Failed to replace old config template in SPIFFS");
        return false;
    }
    
    Serial.println("[DEBUG] Successfully saved config to SPIFFS");
    return true;
}

bool ConfigManager::loadConfig() {
    // Make sure SD card is initialized
    if (!SD.begin(SD_CS)) {
        Serial.println("SD card not initialized, cannot load config");
        return false;
    }
    
    // Check if config file exists
    if (!SD.exists(CONFIG_FILE)) {
        Serial.println("Config file does not exist");
        return false;
    }
    
    // Open config file for reading
    File configFile = SD.open(CONFIG_FILE, FILE_READ);
    if (!configFile) {
        Serial.println("Failed to open config file for reading");
        return false;
    }
    
    size_t size = configFile.size();
    if (size == 0) {
        Serial.println("Config file is empty");
        configFile.close();
        return false;
    }
    
    // Allocate a temporary JsonDocument
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    
    if (error) {
        Serial.print("Failed to parse config file: ");
        Serial.println(error.c_str());
        return false;
    }
    
    return parseConfig(doc);
}

bool ConfigManager::saveConfig() {
    // Make sure SD card is initialized
    if (!SD.begin(SD_CS)) {
        Serial.println("SD card not initialized, cannot save config");
        return false;
    }
    
    // Create a temporary file
    File tempFile = SD.open("/temp_config.json", FILE_WRITE);
    if (!tempFile) {
        Serial.println("Failed to create temp config file");
        return false;
    }
    
    // Create JSON document
    DynamicJsonDocument doc(4096);
    
    // WiFi
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["ssid"] = wifiConfig.ssid;
    wifi["password"] = wifiConfig.password;
    
    // NTP
    JsonObject ntp = doc.createNestedObject("ntp");
    ntp["server"] = ntpConfig.server;
    ntp["timezone"] = ntpConfig.timezone;
    
    // Display
    JsonObject display = doc.createNestedObject("display");
    display["brightness"] = displayConfig.brightness;
    display["timeout"] = displayConfig.timeout;
    display["auto_brightness"] = displayConfig.auto_brightness;
    display["theme"] = displayConfig.theme;
    
    // Alarms
    JsonArray alarmsArray = doc.createNestedArray("alarms");
    for (const auto& alarm : alarms) {
        JsonObject alarmObj = alarmsArray.createNestedObject();
        alarmObj["id"] = alarm.id;
        alarmObj["enabled"] = alarm.enabled;
        alarmObj["hour"] = alarm.hour;
        alarmObj["minute"] = alarm.minute;
        
        JsonArray daysArray = alarmObj.createNestedArray("days");
        for (bool day : alarm.days) {
            daysArray.add(day);
        }
        
        alarmObj["type"] = alarm.type;
        alarmObj["station_id"] = alarm.station_id;
        alarmObj["volume"] = alarm.volume;
        alarmObj["fade_in"] = alarm.fade_in;
        alarmObj["duration"] = alarm.duration;
    }
    
    // Radio Stations
    JsonArray stationsArray = doc.createNestedArray("radio_stations");
    for (const auto& station : radioStations) {
        JsonObject stationObj = stationsArray.createNestedObject();
        stationObj["id"] = station.id;
        stationObj["name"] = station.name;
        stationObj["url"] = station.url;
        stationObj["genre"] = station.genre;
    }
    
    // Weather
    JsonObject weather = doc.createNestedObject("weather");
    weather["api_key"] = weatherConfig.api_key;
    weather["city_id"] = weatherConfig.city_id;
    weather["units"] = weatherConfig.units;
    weather["update_interval"] = weatherConfig.update_interval;
    
    // System
    JsonObject system = doc.createNestedObject("system");
    system["hostname"] = systemConfig.hostname;
    system["ota_password"] = systemConfig.ota_password;
    
    // Fallback audio
    doc["fallback_audio"] = fallbackAudio;
    
    // Serialize JSON to file
    if (serializeJson(doc, tempFile) == 0) {
        Serial.println("Failed to write to temp config file");
        tempFile.close();
        SD.remove("/temp_config.json");
        return false;
    }
    
    tempFile.close();
    
    // Close the temp file before renaming
    tempFile.close();
    
    // Remove old config if it exists
    if (SD.exists(CONFIG_FILE)) {
        if (!SD.remove(CONFIG_FILE)) {
            Serial.println("Failed to remove old config file");
            SD.remove("/temp_config.json");
            return false;
        }
    }
    
    // Rename temp file to config file
    if (!SD.rename("/temp_config.json", CONFIG_FILE)) {
        Serial.println("Failed to rename temp config file");
        SD.remove("/temp_config.json");
        return false;
    }
    
    return true;
}

bool ConfigManager::resetToDefault() {
    setDefaultConfig();
    return saveConfig();
}

bool ConfigManager::parseConfig(JsonDocument& doc) {
    // WiFi
    wifiConfig.ssid = doc["wifi"]["ssid"].as<String>();
    wifiConfig.password = doc["wifi"]["password"].as<String>();
    
    // NTP
    ntpConfig.server = doc["ntp"]["server"].as<String>();
    ntpConfig.timezone = doc["ntp"]["timezone"].as<String>();
    
    // Display
    displayConfig.brightness = doc["display"]["brightness"] | 100;
    displayConfig.timeout = doc["display"]["timeout"] | 30;
    displayConfig.auto_brightness = doc["display"]["auto_brightness"] | true;
    displayConfig.theme = doc["display"]["theme"].as<String>();
    
    // Alarms
    alarms.clear();
    JsonArray alarmsArray = doc["alarms"];
    for (JsonObject alarmObj : alarmsArray) {
        AlarmConfig alarm;
        alarm.id = alarmObj["id"];
        alarm.enabled = alarmObj["enabled"];
        alarm.hour = alarmObj["hour"];
        alarm.minute = alarmObj["minute"];
        
        JsonArray daysArray = alarmObj["days"];
        for (int i = 0; i < 7 && i < daysArray.size(); i++) {
            alarm.days[i] = daysArray[i];
        }
        
        alarm.type = alarmObj["type"].as<String>();
        alarm.station_id = alarmObj["station_id"];
        alarm.volume = alarmObj["volume"];
        alarm.fade_in = alarmObj["fade_in"];
        alarm.duration = alarmObj["duration"];
        
        alarms.push_back(alarm);
    }
    
    // Radio Stations
    radioStations.clear();
    JsonArray stationsArray = doc["radio_stations"];
    for (JsonObject stationObj : stationsArray) {
        RadioStation station;
        station.id = stationObj["id"];
        station.name = stationObj["name"].as<String>();
        station.url = stationObj["url"].as<String>();
        station.genre = stationObj["genre"].as<String>();
        
        radioStations.push_back(station);
    }
    
    // Weather
    weatherConfig.api_key = doc["weather"]["api_key"].as<String>();
    weatherConfig.city_id = doc["weather"]["city_id"].as<String>();
    weatherConfig.units = doc["weather"]["units"].as<String>();
    weatherConfig.update_interval = doc["weather"]["update_interval"] | 30;
    
    // System
    systemConfig.hostname = doc["system"]["hostname"].as<String>();
    systemConfig.ota_password = doc["system"]["ota_password"].as<String>();
    
    // Fallback audio
    fallbackAudio = doc["fallback_audio"].as<String>();
    
    return true;
}

void ConfigManager::setDefaultConfig() {
    // WiFi
    wifiConfig.ssid = "";
    wifiConfig.password = "";
    
    // NTP
    ntpConfig.server = "pool.ntp.org";
    ntpConfig.timezone = "CET-1CEST,M3.5.0,M10.5.0/3";
    
    // Display
    displayConfig.brightness = 100;
    displayConfig.timeout = 30;
    displayConfig.auto_brightness = true;
    displayConfig.theme = "dark";
    
    // Default alarm (7:00 AM on weekdays)
    AlarmConfig defaultAlarm;
    defaultAlarm.id = 1;
    defaultAlarm.enabled = true;
    defaultAlarm.hour = 7;
    defaultAlarm.minute = 0;
    for (int i = 0; i < 7; i++) {
        defaultAlarm.days[i] = (i >= 1 && i <= 5); // Weekdays
    }
    defaultAlarm.type = "radio";
    defaultAlarm.station_id = 0;
    defaultAlarm.volume = 70;
    defaultAlarm.fade_in = 30;
    defaultAlarm.duration = 60;
    
    alarms.clear();
    alarms.push_back(defaultAlarm);
    
    // Default radio station
    RadioStation defaultStation;
    defaultStation.id = 0;
    defaultStation.name = "Example Radio";
    defaultStation.url = "http://example.com/stream.mp3";
    defaultStation.genre = "Various";
    
    radioStations.clear();
    radioStations.push_back(defaultStation);
    
    // Weather
    weatherConfig.api_key = "";
    weatherConfig.city_id = "";
    weatherConfig.units = "metric";
    weatherConfig.update_interval = 30;
    
    // System
    systemConfig.hostname = "radiowecker";
    systemConfig.ota_password = "changeme";
    
    // Fallback audio
    fallbackAudio = "/alarm.mp3";
}
