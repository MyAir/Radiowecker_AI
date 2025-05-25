#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "ConfigManager.h"
#include "DisplayManager.h"

// Forward declare LVGL types we need
typedef struct _lv_obj_t lv_obj_t;
typedef struct _lv_event_t lv_event_t;
typedef struct _lv_obj_class_t lv_obj_class_t;

typedef void (*ui_event_cb_t)(lv_event_t * e);

// Forward declarations
class UIManager;
class DisplayManager;
class AudioManager;

class UIManager {
private:
    static UIManager* instance;
    
    // Private constructor
    UIManager() = default;
    
    // Prevent copying and assignment
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;
    
public:
    static UIManager& getInstance() {
        if (!instance) {
            instance = new UIManager();
        }
        return *instance;
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
    
    /**
     * @brief Update the WiFi SSID display
     * @param ssid WiFi network name
     */
    void updateWifiSsid(const char* ssid);
    
    /**
     * @brief Update the IP address display
     * @param ipAddress IP address as string
     */
    void updateIpAddress(const char* ipAddress);
    
    /**
     * @brief Update the WiFi signal quality display
     * @param quality Signal quality (0-100%)
     */
    void updateWifiQuality(int quality);
    
    /**
     * @brief Update the current weather display
     * @param icon Icon code (from OpenWeatherMap)
     * @param temperature Current temperature in Celsius
     * @param feelsLike Perceived temperature in Celsius
     */
    void updateCurrentWeather(const char* icon, float temperature, float feelsLike);
    
    /**
     * @brief Update the morning forecast display
     * @param icon Icon code (from OpenWeatherMap)
     * @param temperature Temperature in Celsius
     * @param rainProb Rain probability (0-100%)
     */
    void updateMorningForecast(const char* icon, float temperature, int rainProb);
    
    /**
     * @brief Update the afternoon forecast display
     * @param icon Icon code (from OpenWeatherMap)
     * @param temperature Temperature in Celsius
     * @param rainProb Rain probability (0-100%)
     */
    void updateAfternoonForecast(const char* icon, float temperature, int rainProb);
    
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
    lv_obj_t* currentScreen = nullptr; // Currently active screen
    
    // Home screen elements
    lv_obj_t* timeLabel = nullptr;
    lv_obj_t* dateLabel = nullptr;
    lv_obj_t* tempLabel = nullptr;
    lv_obj_t* humidityLabel = nullptr;
    lv_obj_t* tvocLabel = nullptr;
    lv_obj_t* eco2Label = nullptr;
    lv_obj_t* nextAlarmLabel = nullptr;
    lv_obj_t* currentAlarmScreen = nullptr;
    
    // Weather elements
    lv_obj_t* weatherPanel = nullptr;
    
    // Current weather
    lv_obj_t* currentWeatherPanel = nullptr;
    lv_obj_t* currentWeatherIcon = nullptr;
    lv_obj_t* currentTempLabel = nullptr;
    lv_obj_t* feelsLikeLabel = nullptr;
    
    // Morning forecast
    lv_obj_t* morningForecastPanel = nullptr;
    lv_obj_t* morningForecastIcon = nullptr;
    lv_obj_t* morningTempLabel = nullptr;
    lv_obj_t* morningRainLabel = nullptr;
    
    // Afternoon forecast
    lv_obj_t* afternoonForecastPanel = nullptr;
    lv_obj_t* afternoonForecastIcon = nullptr;
    lv_obj_t* afternoonTempLabel = nullptr;
    lv_obj_t* afternoonRainLabel = nullptr;
    
    // Status bar elements
    lv_obj_t* wifiSsidLabel = nullptr;
    lv_obj_t* ipAddressLabel = nullptr;
    lv_obj_t* wifiQualityLabel = nullptr;
    
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
    
public:
    // Public methods for callbacks
    void triggerAlarmCallback(bool enabled, uint8_t hour, uint8_t minute, bool* days) {
        if (alarmCallback) {
            alarmCallback(enabled, hour, minute, days);
        }
    }
    
    void triggerVolumeCallback(uint8_t volume) {
        if (volumeCallback) {
            volumeCallback(volume);
        }
    }
    
    void setBrightness(uint8_t brightness) {
        if (brightnessCallback) {
            brightnessCallback(brightness);
        }
    }
    
        // Structure for passing data to LVGL callbacks
    struct UserData {
        UIManager* ui;
        int value;  // Can be used for various purposes (e.g., day index)
    };
    
    // Static callbacks for LVGL - these are C-style callback functions
    static void alarm_toggle_cb(lv_event_t* e);
    static void volume_changed_cb(lv_event_t* e);
    static void radio_volume_changed_cb(lv_event_t* e);
    static void brightness_changed_cb(lv_event_t* e);
    static void back_btn_clicked_cb(lv_event_t* e);
    static void theme_switch_cb(lv_event_t* e);
    static void day_btn_clicked_cb(lv_event_t* e);
    static void save_alarm_cb(lv_event_t* e);
    static void nav_btn_clicked_cb(lv_event_t* e);
};

// Initialize static member
inline UIManager* UIManager::instance = nullptr;
