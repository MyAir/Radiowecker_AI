#pragma once

#include <Arduino.h>
#include <time.h>
#include <vector>
#include <functional>

struct Alarm {
    uint8_t id;
    uint8_t hour;
    uint8_t minute;
    bool enabled;
    bool repeat[7];  // 0=Sunday, 1=Monday, ..., 6=Saturday
    uint8_t volume;  // 0-100
    uint8_t source;  // 0=Radio, 1=MP3
    union {
        uint8_t stationIndex;  // For radio
        char filepath[64];     // For MP3 files
    } sourceData;
    
    bool shouldTrigger(const struct tm& timeInfo) const;
    void getNextTriggerTime(struct tm& timeInfo) const;
};

class AlarmManager {
private:
    static AlarmManager* instance;
    
    // Private constructor
    AlarmManager() = default;
    
    // Prevent copying and assignment
    AlarmManager(const AlarmManager&) = delete;
    AlarmManager& operator=(const AlarmManager&) = delete;
    
public:
    static constexpr uint8_t MAX_ALARMS = 10;
    
    // Alarm trigger callback type
    using AlarmTriggerCallback = std::function<void(const Alarm& alarm)>;
    
    static AlarmManager& getInstance() {
        if (!instance) {
            instance = new AlarmManager();
        }
        return *instance;
    }
    
    void begin();
    void update();
    
    // Alarm management
    bool addAlarm(const Alarm& alarm);
    bool updateAlarm(const Alarm& alarm);
    bool removeAlarm(uint8_t id);
    const Alarm* getAlarm(uint8_t id) const;
    const std::vector<Alarm>& getAlarms() const { return alarms; }
    
    // Snooze functionality
    void snoozeCurrentAlarm(uint8_t minutes = 5);
    void stopCurrentAlarm();
    
    // Callbacks
    void setAlarmTriggerCallback(AlarmTriggerCallback cb) { triggerCallback = cb; }
    
    // Time management
    void setTimeZone(const char* tz);
    bool isTimeSet() const { return timeSet; }
    
    // Snooze state
    bool isSnoozing() const { return snoozeEndTime != 0; }
    uint32_t getSnoozeRemaining() const;
    
    // Alarm checking (called from task)
    void checkAlarms();
    
private:
    // AlarmManager() = default;
    // ~AlarmManager() = default;
    
    // Delete copy constructor and assignment operator
    // AlarmManager(const AlarmManager&) = delete;
    // AlarmManager& operator=(const AlarmManager&) = delete;
    
    std::vector<Alarm> alarms;
    bool timeSet = false;
    time_t lastCheckTime = 0;
    time_t snoozeEndTime = 0;
    uint8_t lastTriggeredAlarmId = 0;
    
    // Callbacks
    AlarmTriggerCallback triggerCallback = nullptr;
    
    // Private helpers
    bool isAlarmActive(const Alarm& alarm) const;
    void loadAlarms();
    void saveAlarms();
};

// Initialize static member
// inline AlarmManager* AlarmManager::instance = nullptr;
