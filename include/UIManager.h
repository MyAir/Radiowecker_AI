#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include "ConfigManager.h"
#include "DisplayManager.h"
#include "WeatherIcons.h"

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
     * @brief Update the current weather display
     * @param temp Current temperature in °C
     * @param feels_like Perceived temperature in °C
     * @param description Weather description text
     * @param iconCode Weather icon code from OpenWeatherMap
     */
    void updateCurrentWeather(float temp, float feels_like, const char* description, const char* iconCode);
    
    /**
     * @brief Update the morning forecast display
     * @param temp Temperature in °C
     * @param pop Probability of precipitation (0-1)
     * @param iconCode Weather icon code from OpenWeatherMap
     */
    void updateMorningForecast(float temp, float pop, const char* iconCode);
    
    /**
     * @brief Update the afternoon forecast display
     * @param temp Temperature in °C
     * @param pop Probability of precipitation (0-1)
     * @param iconCode Weather icon code from OpenWeatherMap
     */
    void updateAfternoonForecast(float temp, float pop, const char* iconCode);
    
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
    
    // LVGL screen objects - made public for direct access from lambda functions
    lv_obj_t* homeScreen = nullptr;
    lv_obj_t* alarmSettingsScreen = nullptr;
    lv_obj_t* radioScreen = nullptr;
    lv_obj_t* settingsScreen = nullptr;
    lv_obj_t* settingsBackArea = nullptr; // Transparent back area at the top of settings screen
    lv_obj_t* alarmScreen = nullptr;
    lv_obj_t* currentScreen = nullptr; // Currently active screen
    
private:
    // Theme
    void initTheme();
    
    // Screen creation
    void createHomeScreen();
    void createAlarmSettingsScreen();
    void createRadioScreen();
    void createSettingsScreen();
    
    // Home screen elements
    lv_obj_t* timeLabel = nullptr;
    lv_obj_t* dateLabel = nullptr;
    lv_obj_t* nextAlarmLabel = nullptr;
    lv_obj_t* wifiLabel = nullptr;
    lv_obj_t* ipLabel = nullptr;
    lv_obj_t* wifiSsidLabel = nullptr;
    lv_obj_t* tempLabel = nullptr;
    lv_obj_t* humidityLabel = nullptr;
    lv_obj_t* tvocLabel = nullptr;
    lv_obj_t* eco2Label = nullptr;
    lv_obj_t* ipAddressLabel = nullptr;
    lv_obj_t* currentAlarmScreen = nullptr;
    
    // Weather panel elements
    lv_obj_t* weatherPanel = nullptr;
    lv_obj_t* currentWeatherTitle = nullptr;
    lv_obj_t* currentTempLabel = nullptr;
    lv_obj_t* feelsLikeLabel = nullptr;
    lv_obj_t* weatherDescLabel = nullptr;
    lv_obj_t* weatherIcon = nullptr;
    lv_obj_t* weatherIconImg = nullptr;
    
    // Forecast elements
    lv_obj_t* forecastPanel = nullptr;
    lv_obj_t* morningTitle = nullptr;
    lv_obj_t* afternoonTitle = nullptr;
    lv_obj_t* morningTempLabel = nullptr;
    lv_obj_t* morningRainLabel = nullptr;
    lv_obj_t* morningIcon = nullptr;
    lv_obj_t* morningIconImg = nullptr;
    lv_obj_t* afternoonIcon = nullptr;
    lv_obj_t* afternoonIconImg = nullptr;
    lv_obj_t* afternoonTempLabel = nullptr;
    lv_obj_t* afternoonRainLabel = nullptr;
    
    // Status bar elements
    lv_obj_t* wifiQualityLabel = nullptr;
    
    // Callbacks
    AlarmCallback alarmCallback = nullptr;
    VolumeCallback volumeCallback = nullptr;
    BrightnessCallback brightnessCallback = nullptr;
    
    // Styles
    lv_style_t infoStyle;     // Style for general information text
    lv_style_t statusStyle;   // Style for status bar text
    lv_style_t timeStyle;     // Style for the clock
    lv_style_t dateStyle;     // Style for the date
    lv_style_t panelStyle;    // Style for panels
    lv_style_t titleStyle;    // Style for titles
    lv_style_t valueStyle;    // Style for values
    lv_style_t iconStyle;     // Style for icons
    lv_style_t weatherIconStyle; // Style for weather icons
    lv_style_t buttonStyle;   // Style for buttons
    lv_style_t buttonPressedStyle; // Style for pressed buttons
    lv_style_t dayButtonStyle;     // Style for day buttons in alarm settings
    lv_style_t dayButtonActiveStyle; // Style for active day buttons
    
    // Current theme
    bool darkTheme = true;
    
    // Timer for settings screen timeout
    lv_timer_t* settingsScreenTimer = nullptr;
    
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
    
    // New screen navigation callbacks
    static void back_area_clicked_cb(lv_event_t* e);
    static void alarm_btn_clicked_cb(lv_event_t* e);
    static void radio_btn_clicked_cb(lv_event_t* e);
    static void weather_btn_clicked_cb(lv_event_t* e);
};

// Static member will be defined in the cpp file
