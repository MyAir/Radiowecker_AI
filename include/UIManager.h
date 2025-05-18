#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "ConfigManager.h"

// Forward declare LVGL types we need
typedef struct _lv_obj_t lv_obj_t;
typedef struct _lv_event_t lv_event_t;
typedef struct _lv_style_t lv_style_t;
typedef struct _lv_obj_class_t lv_obj_class_t;

typedef void (*ui_event_cb_t)(lv_event_t * e);

// Forward declarations for our own types
class UIManager;

class UIManager {
public:
    static UIManager& getInstance() {
        static UIManager instance;
        return instance;
    }

    /**
     * @brief Initialize the UI Manager
     * @return true if initialization was successful, false otherwise
     */
    bool init();
    
    /**
     * @brief Show the main screen
     */
    void showMainScreen();
    
    /**
     * @brief Update the time display
     * @param timeStr Time string in HH:MM format
     */
    void updateTime(const char* timeStr);
    
    /**
     * @brief Update the date display
     * @param dateStr Date string in Weekday, DD Month format
     */
    void updateDate(const char* dateStr);
    
    /**
     * @brief Show a message on the screen
     * @param title Message title
     * @param message Message content
     */
    void showMessage(const char* title, const char* message);
    
    /**
     * @brief Show the alarm screen
     */
    void showAlarmScreen();
    
    /**
     * @brief Hide the alarm screen
     */
    void hideAlarmScreen();
    
    /**
     * @brief Update the volume display
     * @param volume Volume level (0-100)
     */
    void updateVolume(uint8_t volume);
    
    /**
     * @brief Update the brightness display
     * @param brightness Brightness level (0-100)
     */
    void updateBrightness(uint8_t brightness);
    
    /**
     * @brief Update the temperature display
     * @param temp Temperature value
     */
    void updateTemperature(float temp);
    
    /**
     * @brief Update the humidity display
     * @param humidity Humidity value (0-100%)
     */
    void updateHumidity(float humidity);
    
    /**
     * @brief Update the TVOC display
     * @param tvoc TVOC value in ppb
     */
    void updateTVOC(uint16_t tvoc);
    
    /**
     * @brief Update the eCO2 display
     * @param eco2 eCO2 value in ppm
     */
    void updateCO2(uint16_t eco2);
    
    /**
     * @brief Update the next alarm display
     * @param hour Alarm hour (0-23)
     * @param minute Alarm minute (0-59)
     * @param enabled Whether the alarm is enabled
     */
    void updateNextAlarm(uint8_t hour, uint8_t minute, bool enabled = true);
    
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
    lv_obj_t* homeScreen = nullptr;
    lv_obj_t* alarmSettingsScreen = nullptr;
    lv_obj_t* radioScreen = nullptr;
    lv_obj_t* settingsScreen = nullptr;
    lv_obj_t* alarmScreen = nullptr;
    
    // Home screen elements
    lv_obj_t* timeLabel = nullptr;
    lv_obj_t* dateLabel = nullptr;
    lv_obj_t* tempLabel = nullptr;
    lv_obj_t* humidityLabel = nullptr;
    lv_obj_t* tvocLabel = nullptr;
    lv_obj_t* eco2Label = nullptr;
    lv_obj_t* nextAlarmLabel = nullptr;
    lv_obj_t* currentAlarmScreen = nullptr;
    lv_obj_t* weatherIcon = nullptr;
    lv_obj_t* feelsLikeLabel = nullptr;
    
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
    
    // Callback getters
    AlarmCallback& getAlarmCallback() { return alarmCallback; }
    VolumeCallback& getVolumeCallback() { return volumeCallback; }
    BrightnessCallback& getBrightnessCallback() { return brightnessCallback; }
    
    // Static callbacks
    static void alarm_toggle_cb(lv_event_t* e);
    static void volume_changed_cb(lv_event_t* e);
    static void brightness_changed_cb(lv_event_t* e);
};

extern UIManager& uiManager;
