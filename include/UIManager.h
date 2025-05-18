#pragma once

#include <lvgl.h>
#include "ConfigManager.h"

class UIManager {
public:
    static UIManager& getInstance() {
        static UIManager instance;
        return instance;
    }

    void begin();
    void updateTime();
    void updateWeather(const String& condition, float temp, float feels_like, int humidity);
    void updateAirQuality(float tvoc, float eco2, float temp, float humidity);
    void showAlarmScreen();
    void hideAlarmScreen();
    void showMessage(const String& title, const String& message);
    void updateVolume(uint8_t volume);
    void updateBrightness(uint8_t brightness);
    
    // Callback types
    typedef void (*AlarmCallback)(bool enabled, uint8_t hour, uint8_t minute, bool days[7]);
    typedef void (*VolumeCallback)(uint8_t volume);
    typedef void (*BrightnessCallback)(uint8_t brightness);
    
    // Set callbacks
    void setAlarmCallback(AlarmCallback cb) { alarmCallback = cb; }
    void setVolumeCallback(VolumeCallback cb) { volumeCallback = cb; }
    void setBrightnessCallback(BrightnessCallback cb) { brightnessCallback = cb; }
    
    // Screen management
    void showScreen(lv_obj_t* screen);
    void showHomeScreen();
    void showAlarmSettingsScreen();
    void showRadioScreen();
    void showSettingsScreen();
    
private:
    UIManager() = default;
    ~UIManager() = default;
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    
    // Theme
    void initTheme();
    
    // Screen creation
    void createHomeScreen();
    void createAlarmSettingsScreen();
    void createRadioScreen();
    void createSettingsScreen();
    
    // UI elements
    lv_obj_t* homeScreen;
    lv_obj_t* alarmSettingsScreen;
    lv_obj_t* radioScreen;
    lv_obj_t* settingsScreen;
    
    // Home screen elements
    lv_obj_t* timeLabel;
    lv_obj_t* dateLabel;
    lv_obj_t* nextAlarmLabel;
    lv_obj_t* weatherIcon;
    lv_obj_t* tempLabel;
    lv_obj_t* feelsLikeLabel;
    lv_obj_t* tvocLabel;
    lv_obj_t* eco2Label;
    lv_obj_t* humidityLabel;
    
    // Callbacks
    AlarmCallback alarmCallback = nullptr;
    VolumeCallback volumeCallback = nullptr;
    BrightnessCallback brightnessCallback = nullptr;
    
    // Styles
    lv_style_t timeStyle;
    lv_style_t dateStyle;
    lv_style_t infoStyle;
    lv_style_t buttonStyle;
    lv_style_t buttonPressedStyle;
    
    // Current theme
    bool darkTheme = true;
    
    // Friend declaration for static callbacks
    friend void alarm_toggle_cb(lv_event_t* e);
    friend void volume_changed_cb(lv_event_t* e);
    friend void brightness_changed_cb(lv_event_t* e);
};

extern UIManager& uiManager;
