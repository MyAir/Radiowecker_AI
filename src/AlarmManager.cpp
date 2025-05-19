#include "AlarmManager.h"
#include "ConfigManager.h"
#include <time.h>
#include <ArduinoJson.h>
#include <SD_MMC.h>

// Initialize static member
AlarmManager* AlarmManager::instance = nullptr;

// Alarm methods
bool Alarm::shouldTrigger(const struct tm& timeInfo) const {
    if (!enabled) return false;
    
    // Check if alarm time matches current time
    if (timeInfo.tm_hour != hour || timeInfo.tm_min != minute) {
        return false;
    }
    
    // Check if alarm repeats on this day of week
    if (repeat[timeInfo.tm_wday]) {
        return true;
    }
    
    return false;
}

void Alarm::getNextTriggerTime(struct tm& timeInfo) const {
    if (!enabled) {
        // Return max time if alarm is disabled
        timeInfo.tm_year = 2037 - 1900; // Year 2037
        timeInfo.tm_mon = 12 - 1;       // December
        timeInfo.tm_mday = 31;          // 31st
        timeInfo.tm_hour = 23;
        timeInfo.tm_min = 59;
        timeInfo.tm_sec = 59;
        return;
    }
    
    time_t now;
    time(&now);
    struct tm next = *localtime(&now);
    
    // Set the target time
    next.tm_hour = hour;
    next.tm_min = minute;
    next.tm_sec = 0;
    
    // If the time has already passed today, move to next day
    time_t nextTime = mktime(&next);
    if (nextTime <= now) {
        next.tm_mday++;
        nextTime = mktime(&next);
    }
    
    // Find the next day when the alarm is set
    bool found = false;
    for (int i = 0; i < 7; i++) {
        int wday = (next.tm_wday + i) % 7;
        if (repeat[wday]) {
            next.tm_mday += i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        // No repeat days set, use max time
        next.tm_year = 2037 - 1900;
        next.tm_mon = 12 - 1;
        next.tm_mday = 31;
        next.tm_hour = 23;
        next.tm_min = 59;
        next.tm_sec = 59;
    } else {
        nextTime = mktime(&next);
    }
    
    timeInfo = *localtime(&nextTime);
}

// AlarmManager implementation
void AlarmManager::begin() {
    loadAlarms();
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
    
    // Wait for time sync (non-blocking, will check in update())
    timeSet = false;
    lastCheckTime = 0;
}

void AlarmManager::update() {
    time_t now;
    time(&now);
    
    // Check if time is set (NTP synchronized)
    if (!timeSet) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 100)) {  // 100ms timeout
            timeSet = true;
            Serial.println("Time synchronized");
        } else {
            return;  // Time not set yet
        }
    }
    
    // Check alarms every second
    if (now - lastCheckTime >= 1) {
        lastCheckTime = now;
        checkAlarms();
    }
    
    // Check snooze timeout
    if (snoozeEndTime > 0 && now >= snoozeEndTime) {
        snoozeEndTime = 0;
        // Re-trigger the last alarm
        if (lastTriggeredAlarmId > 0) {
            for (const auto& alarm : alarms) {
                if (alarm.id == lastTriggeredAlarmId) {
                    if (triggerCallback) {
                        triggerCallback(alarm);
                    }
                    break;
                }
            }
        }
    }
}

bool AlarmManager::addAlarm(const Alarm& alarm) {
    if (alarms.size() >= MAX_ALARMS) {
        return false;
    }
    
    // Check for duplicate ID
    for (const auto& a : alarms) {
        if (a.id == alarm.id) {
            return false;
        }
    }
    
    alarms.push_back(alarm);
    saveAlarms();
    return true;
}

bool AlarmManager::updateAlarm(const Alarm& alarm) {
    for (auto& a : alarms) {
        if (a.id == alarm.id) {
            a = alarm;
            saveAlarms();
            return true;
        }
    }
    return false;
}

bool AlarmManager::removeAlarm(uint8_t id) {
    for (auto it = alarms.begin(); it != alarms.end(); ++it) {
        if (it->id == id) {
            alarms.erase(it);
            saveAlarms();
            return true;
        }
    }
    return false;
}

const Alarm* AlarmManager::getAlarm(uint8_t id) const {
    for (const auto& alarm : alarms) {
        if (alarm.id == id) {
            return &alarm;
        }
    }
    return nullptr;
}

void AlarmManager::snoozeCurrentAlarm(uint8_t minutes) {
    time_t now;
    time(&now);
    snoozeEndTime = now + (minutes * 60);
    lastTriggeredAlarmId = 0;  // Clear last triggered to prevent re-triggering
}

void AlarmManager::stopCurrentAlarm() {
    snoozeEndTime = 0;
    lastTriggeredAlarmId = 0;
}

void AlarmManager::setTimeZone(const char* tz) {
    setenv("TZ", tz, 1);
    tzset();
}

uint32_t AlarmManager::getSnoozeRemaining() const {
    if (snoozeEndTime == 0) {
        return 0;
    }
    
    time_t now;
    time(&now);
    
    if (now >= snoozeEndTime) {
        return 0;
    }
    
    return snoozeEndTime - now;
}

void AlarmManager::checkAlarms() {
    if (!timeSet || alarms.empty()) {
        return;
    }
    
    // Skip if we're in snooze period
    if (snoozeEndTime > 0) {
        time_t now;
        time(&now);
        if (now < snoozeEndTime) {
            return;
        }
        snoozeEndTime = 0;  // Snooze period ended
    }
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return;
    }
    
    // Check each alarm
    for (const auto& alarm : alarms) {
        if (alarm.shouldTrigger(timeinfo)) {
            // Only trigger once per minute
            static time_t lastTriggerMinute = 0;
            time_t currentMinute = timeinfo.tm_min + (timeinfo.tm_hour * 60);
            
            if (currentMinute != lastTriggerMinute) {
                lastTriggerMinute = currentMinute;
                lastTriggeredAlarmId = alarm.id;
                
                if (triggerCallback) {
                    triggerCallback(alarm);
                }
            }
            break;  // Only trigger one alarm per minute
        }
    }
}

void AlarmManager::loadAlarms() {
    if (!SD_MMC.begin("/sdcard", true, true)) {
        Serial.println("SD_MMC initialization failed, cannot load alarms");
        return;
    }
    
    File file = SD_MMC.open("/alarms.json", FILE_READ);
    if (!file) {
        Serial.println("No alarms.json file found, starting with no alarms");
        return;
    }
    
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        Serial.printf("Failed to parse alarms.json: %s\n", error.c_str());
        return;
    }
    
    alarms.clear();
    
    JsonArray alarmsArray = doc.as<JsonArray>();
    for (JsonObject alarmObj : alarmsArray) {
        Alarm alarm;
        alarm.id = alarmObj["id"];
        alarm.hour = alarmObj["hour"];
        alarm.minute = alarmObj["minute"];
        alarm.enabled = alarmObj["enabled"];
        
        JsonArray daysArray = alarmObj["repeat"];
        for (int i = 0; i < 7; i++) {
            alarm.repeat[i] = daysArray[i];
        }
        
        alarm.volume = alarmObj["volume"];
        alarm.source = alarmObj["source"];
        
        if (alarm.source == 0) { // Radio
            alarm.sourceData.stationIndex = alarmObj["stationIndex"];
        } else { // MP3
            strncpy(alarm.sourceData.filepath, alarmObj["filepath"], sizeof(alarm.sourceData.filepath) - 1);
        }
        
        alarms.push_back(alarm);
    }
    
    Serial.printf("Loaded %d alarms\n", alarms.size());
}

void AlarmManager::saveAlarms() {
    if (!SD_MMC.begin("/sdcard", true, true)) {
        Serial.println("SD_MMC initialization failed, cannot save alarms");
        return;
    }
    
    DynamicJsonDocument doc(2048);
    JsonArray alarmsArray = doc.to<JsonArray>();
    
    for (const auto& alarm : alarms) {
        JsonObject alarmObj = alarmsArray.createNestedObject();
        alarmObj["id"] = alarm.id;
        alarmObj["hour"] = alarm.hour;
        alarmObj["minute"] = alarm.minute;
        alarmObj["enabled"] = alarm.enabled;
        
        JsonArray daysArray = alarmObj.createNestedArray("repeat");
        for (int i = 0; i < 7; i++) {
            daysArray.add(alarm.repeat[i]);
        }
        
        alarmObj["volume"] = alarm.volume;
        alarmObj["source"] = alarm.source;
        
        if (alarm.source == 0) { // Radio
            alarmObj["stationIndex"] = alarm.sourceData.stationIndex;
        } else { // MP3
            alarmObj["filepath"] = alarm.sourceData.filepath;
        }
    }
    
    File file = SD_MMC.open("/alarms.json", FILE_WRITE);
    if (!file) {
        Serial.println("Failed to create alarms.json");
        return;
    }
    
    serializeJson(doc, file);
    file.close();
}
