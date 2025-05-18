#include "UIManager.h"
#include "lvgl_helper.h"
#include <Arduino.h>
#include <Arduino_GFX.h>
#include <SPIFFS.h>
#include <FS.h>
#include <WiFi.h>

// Global instance
UIManager& uiManager = UIManager::getInstance();

bool UIManager::init() {
    // Initialize LVGL if not already done
    if (!lv_is_initialized()) {
        lv_init();
    }
    
    // Initialize display and touch
    if (!lvgl_helper_init()) {
        return false;
    }
    
    // Initialize theme
    initTheme();
    
    // Create screens
    createHomeScreen();
    createAlarmSettingsScreen();
    createRadioScreen();
    createSettingsScreen();
    
    // Show home screen by default
    showHomeScreen();
    
    return true;
}

void UIManager::showMainScreen() {
    showHomeScreen();
}

void UIManager::updateTime(const char* timeStr) {
    if (timeLabel) {
        lv_label_set_text(timeLabel, timeStr);
    }
}

void UIManager::updateDate(const char* dateStr) {
    if (dateLabel) {
        lv_label_set_text(dateLabel, dateStr);
    }
}

void UIManager::showMessage(const char* title, const char* message) {
    static lv_obj_t* mbox = nullptr;
    
    if (mbox) {
        lv_msgbox_close(mbox);
        mbox = nullptr;
    }
    
    mbox = lv_msgbox_create(NULL, title, message, NULL, true);
    lv_obj_center(mbox);
}

void UIManager::showAlarmScreen() {
    if (!alarmScreen) {
        alarmScreen = lv_obj_create(NULL);
        lv_obj_set_size(alarmScreen, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(alarmScreen, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_bg_opa(alarmScreen, LV_OPA_70, 0);
        
        lv_obj_t* label = lv_label_create(alarmScreen);
        lv_label_set_text(label, "ALARM!");
        lv_obj_set_style_text_font(label, &lv_font_montserrat_48, 0);
        lv_obj_center(label);
        
        lv_obj_t* btn = lv_btn_create(alarmScreen);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -20);
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            uiManager.hideAlarmScreen();
        }, LV_EVENT_CLICKED, NULL);
        
        label = lv_label_create(btn);
        lv_label_set_text(label, "Dismiss");
        lv_obj_center(label);
    }
    
    lv_scr_load_anim(alarmScreen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
}

void UIManager::hideAlarmScreen() {
    if (alarmScreen) {
        lv_scr_load_anim(homeScreen, LV_SCR_LOAD_ANIM_FADE_OUT, 300, 0, false);
    }
}

void UIManager::updateVolume(uint8_t volume) {
    // Update volume slider if it exists
    lv_obj_t* slider = lv_obj_get_child(radioScreen, 0);
    if (slider && lv_obj_check_type(slider, &lv_slider_class)) {
        lv_slider_set_value(slider, volume, LV_ANIM_OFF);
    }
}

void UIManager::updateBrightness(uint8_t brightness) {
    // Update brightness slider if it exists
    lv_obj_t* slider = lv_obj_get_child(settingsScreen, 1);
    if (slider && lv_obj_check_type(slider, &lv_slider_class)) {
        lv_slider_set_value(slider, brightness, LV_ANIM_OFF);
    }
}

void UIManager::updateTemperature(float temp) {
    if (tempLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f°C", temp);
        lv_label_set_text(tempLabel, buf);
    }
}

void UIManager::updateHumidity(float humidity) {
    if (humidityLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f%%", humidity);
        lv_label_set_text(humidityLabel, buf);
    }
}

void UIManager::updateTVOC(uint16_t tvoc) {
    if (tvocLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "TVOC: %uppb", tvoc);
        lv_label_set_text(tvocLabel, buf);
    }
}

void UIManager::updateCO2(uint16_t eco2) {
    if (eco2Label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "eCO2: %uppm", eco2);
        lv_label_set_text(eco2Label, buf);
    }
}

void UIManager::updateNextAlarm(uint8_t hour, uint8_t minute, bool enabled) {
    if (nextAlarmLabel) {
        char buf[32];
        if (enabled) {
            snprintf(buf, sizeof(buf), "Alarm: %02d:%02d", hour, minute);
        } else {
            snprintf(buf, sizeof(buf), "Alarm: Off");
        }
        lv_label_set_text(nextAlarmLabel, buf);
    }
}

// Static callback implementations
void UIManager::alarm_toggle_cb(lv_event_t* e) {
    lv_obj_t* obj = lv_event_get_target(e);
    bool checked = lv_obj_has_state(obj, LV_STATE_CHECKED);
    
    // Get the alarm time from the parent container
    lv_obj_t* cont = lv_obj_get_parent(obj);
    lv_obj_t* hour_roller = lv_obj_get_child(cont, 1);
    lv_obj_t* min_roller = lv_obj_get_child(cont, 3);
    
    uint8_t hour = lv_roller_get_selected(hour_roller);
    uint8_t minute = lv_roller_get_selected(min_roller);
    
    if (uiManager.getAlarmCallback()) {
        // Default to weekdays only
        bool days[7] = {true, true, true, true, true, false, false};
        uiManager.getAlarmCallback()(checked, hour, minute, days);
    }
}

void UIManager::volume_changed_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int volume = lv_slider_get_value(slider);
    if (uiManager.getVolumeCallback()) {
        uiManager.getVolumeCallback()(volume);
    }
}

void UIManager::brightness_changed_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int brightness = lv_slider_get_value(slider);
    if (uiManager.getBrightnessCallback()) {
        uiManager.getBrightnessCallback()(brightness);
    }
}

void UIManager::createRadioScreen() {
    radioScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(radioScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(radioScreen, lv_color_black(), LV_PART_MAIN);
    
    // Volume slider
    lv_obj_t* volumeSlider = lv_slider_create(radioScreen);
    lv_obj_align(volumeSlider, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_slider_set_range(volumeSlider, 0, 100);
    lv_slider_set_value(volumeSlider, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(volumeSlider, [](lv_event_t* e) {
        lv_obj_t* slider = lv_event_get_target(e);
        int32_t volume = lv_slider_get_value(slider);
        if (uiManager.volumeCallback) {
            uiManager.volumeCallback(volume);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Volume icon
    lv_obj_t* volIcon = lv_label_create(radioScreen);
    lv_label_set_text(volIcon, LV_SYMBOL_VOLUME_MID);
    lv_obj_align_to(volIcon, volumeSlider, LV_ALIGN_OUT_LEFT_MID, -10, 0);
}

void UIManager::createSettingsScreen() {
    settingsScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(settingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(settingsScreen, lv_color_black(), LV_PART_MAIN);
    
    // Back button
    lv_obj_t* btnBack = lv_btn_create(settingsScreen);
    lv_obj_add_style(btnBack, &buttonStyle, 0);
    lv_obj_add_style(btnBack, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnBack, 80, 40);
    lv_obj_align(btnBack, LV_ALIGN_TOP_LEFT, 10, 10);
    
    lv_obj_t* label = lv_label_create(btnBack);
    lv_label_set_text(label, LV_SYMBOL_LEFT);
    lv_obj_center(label);
    lv_obj_add_event_cb(btnBack, [](lv_event_t* e) {
        uiManager.showHomeScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Title
    lv_obj_t* title = lv_label_create(settingsScreen);
    lv_obj_add_style(title, &infoStyle, 0);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Brightness control
    lv_obj_t* brightnessLabel = lv_label_create(settingsScreen);
    lv_label_set_text(brightnessLabel, "Display Brightness");
    lv_obj_align(brightnessLabel, LV_ALIGN_TOP_LEFT, 20, 70);
    
    lv_obj_t* brightnessSlider = lv_slider_create(settingsScreen);
    lv_obj_set_size(brightnessSlider, 300, 20);
    lv_obj_align_to(brightnessSlider, brightnessLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_slider_set_range(brightnessSlider, 10, 100);
    lv_slider_set_value(brightnessSlider, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightnessSlider, brightness_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Auto-brightness switch
    lv_obj_t* autoBrightLabel = lv_label_create(settingsScreen);
    lv_label_set_text(autoBrightLabel, "Auto Brightness");
    lv_obj_align_to(autoBrightLabel, brightnessSlider, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    
    lv_obj_t* autoBrightSwitch = lv_switch_create(settingsScreen);
    lv_obj_align_to(autoBrightSwitch, autoBrightLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_obj_add_state(autoBrightSwitch, LV_STATE_CHECKED);
    
    // Timezone settings
    lv_obj_t* timezoneLabel = lv_label_create(settingsScreen);
    lv_label_set_text(timezoneLabel, "Timezone");
    lv_obj_align_to(timezoneLabel, autoBrightLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 30);
    
    lv_obj_t* timezoneDropdown = lv_dropdown_create(settingsScreen);
    lv_dropdown_set_options(timezoneDropdown, "UTC\nUTC+1 (CET)\nUTC+2 (CEST)\nUTC-5 (EST)\nUTC-8 (PST)");
    lv_obj_align_to(timezoneDropdown, timezoneLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_width(timezoneDropdown, 250);
    
    // WiFi settings button
    lv_obj_t* btnWifi = lv_btn_create(settingsScreen);
    lv_obj_add_style(btnWifi, &buttonStyle, 0);
    lv_obj_add_style(btnWifi, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnWifi, 200, 40);
    lv_obj_align(btnWifi, LV_ALIGN_TOP_MID, 0, 250);
    
    label = lv_label_create(btnWifi);
    lv_label_set_text(label, "WiFi Settings");
    lv_obj_center(label);
    
    // System info
    lv_obj_t* sysInfo = lv_label_create(settingsScreen);
    lv_label_set_text_fmt(sysInfo, "Firmware v1.0.0\nIP: %s\nMAC: %s",
        WiFi.localIP().toString().c_str(),
        WiFi.macAddress().c_str());
    lv_obj_align(sysInfo, LV_ALIGN_BOTTOM_LEFT, 20, -20);
}

void UIManager::showScreen(lv_obj_t* screen) {
    if (screen) {
        currentScreen = screen;
        lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 300, 0, false);
    }
}

void UIManager::showHomeScreen() {
    if (!homeScreen) {
        createHomeScreen();
    }
    showScreen(homeScreen);
}

void UIManager::showAlarmSettingsScreen() {
    if (!alarmSettingsScreen) {
        createAlarmSettingsScreen();
    }
    showScreen(alarmSettingsScreen);
}

void UIManager::showRadioScreen() {
    if (!radioScreen) {
        createRadioScreen();
    }
    showScreen(radioScreen);
}

void UIManager::showSettingsScreen() {
    if (!settingsScreen) {
        createSettingsScreen();
    }
    showScreen(settingsScreen);
}

void UIManager::updateTime(const String& timeStr) {
    if (timeLabel) {
        lv_label_set_text(timeLabel, timeStr.c_str());
    }
}

void UIManager::updateDate(const String& dateStr) {
    if (dateLabel) {
        lv_label_set_text(dateLabel, dateStr.c_str());
    }
}

void UIManager::updateTemperature(const String& tempStr) {
    if (tempLabel) {
        lv_label_set_text_fmt(tempLabel, "Temp: %s°C", tempStr.c_str());
    }
}

void UIManager::updateHumidity(const String& humStr) {
    if (humidityLabel) {
        lv_label_set_text_fmt(humidityLabel, "Humidity: %s%%", humStr.c_str());
    }
}

void UIManager::updateTVOC(const String& tvocStr) {
    if (tvocLabel) {
        lv_label_set_text_fmt(tvocLabel, "TVOC: %s ppb", tvocStr.c_str());
    }
}

void UIManager::updateCO2(const String& co2Str) {
    if (eco2Label) {
        lv_label_set_text_fmt(eco2Label, "eCO2: %s ppm", co2Str.c_str());
    }
}

void UIManager::updateNextAlarm(const String& alarmStr) {
    if (nextAlarmLabel) {
        lv_label_set_text(nextAlarmLabel, alarmStr.c_str());
    }
}

void UIManager::updateWeather(const String& condition, float temp, float feels_like, int humidity) {
    // Update weather icon based on condition
    const char* emoji = "❓";
    String cond = condition;
    cond.toLowerCase();
    
    if (cond.indexOf("clear") >= 0) emoji = "☀️";
    else if (cond.indexOf("cloud") >= 0) emoji = "☁️";
    else if (cond.indexOf("rain") >= 0) emoji = "🌧️";
    else if (cond.indexOf("snow") >= 0) emoji = "❄️";
    else if (cond.indexOf("thunder") >= 0) emoji = "⛈️";
    
    lv_label_set_text(weatherIcon, emoji);
    lv_label_set_text_fmt(tempLabel, "%.1f°C", temp);
    lv_label_set_text_fmt(feelsLikeLabel, "Feels like %.1f°C", feels_like);
}

void UIManager::updateAirQuality(float tvoc, float eco2, float temp, float humidity) {
    lv_label_set_text_fmt(tvocLabel, "TVOC: %.0f ppb", tvoc);
    lv_label_set_text_fmt(eco2Label, "eCO2: %.0f ppm", eco2);
    lv_label_set_text_fmt(humidityLabel, "Humidity: %.1f%%", humidity);
    
    // Update background color based on TVOC level (green to red)
    uint8_t r = map(constrain(tvoc, 0, 1000), 0, 1000, 0, 255);
    uint8_t g = map(constrain(tvoc, 0, 1000), 0, 1000, 255, 0);
    lv_obj_set_style_bg_color(lv_obj_get_parent(tvocLabel), lv_color_make(r, g, 0), 0);
}

void UIManager::showAlarmScreen() {
    // Create a full-screen overlay for the alarm
    lv_obj_t* alarmScreen = lv_obj_create(lv_scr_act());
    lv_obj_set_size(alarmScreen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(alarmScreen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(alarmScreen, LV_OPA_80, 0);
    
    // Alarm time
    lv_obj_t* alarmTime = lv_label_create(alarmScreen);
    lv_obj_add_style(alarmTime, &timeStyle, 0);
    lv_label_set_text(alarmTime, "07:00");
    lv_obj_center(alarmTime);
    
    // Snooze button
    lv_obj_t* btnSnooze = lv_btn_create(alarmScreen);
    lv_obj_add_style(btnSnooze, &buttonStyle, 0);
    lv_obj_add_style(btnSnooze, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnSnooze, 150, 60);
    lv_obj_align(btnSnooze, LV_ALIGN_BOTTOM_MID, -90, -40);
    
    lv_obj_t* label = lv_label_create(btnSnooze);
    lv_label_set_text(label, "Snooze");
    lv_obj_center(label);
    
    // Stop button
    lv_obj_t* btnStop = lv_btn_create(alarmScreen);
    lv_obj_add_style(btnStop, &buttonStyle, 0);
    lv_obj_add_style(btnStop, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnStop, 150, 60);
    lv_obj_align(btnStop, LV_ALIGN_BOTTOM_MID, 90, -40);
    
    label = lv_label_create(btnStop);
    lv_label_set_text(label, "Stop");
    lv_obj_center(label);
    
    // Store the alarm screen reference
    currentAlarmScreen = alarmScreen;
}

void UIManager::hideAlarmScreen() {
    if (currentAlarmScreen) {
        lv_obj_del(currentAlarmScreen);
        currentAlarmScreen = nullptr;
    }
}

void UIManager::showMessage(const String& title, const String& message) {
    // Create a message box
    lv_obj_t* mbox = lv_msgbox_create(NULL, title.c_str(), message.c_str(), NULL, true);
    lv_obj_center(mbox);
    
    // Auto-close after 3 seconds
    lv_timer_create([](lv_timer_t* timer) {
        lv_obj_t* mbox = (lv_obj_t*)timer->user_data;
        lv_msgbox_close(mbox);
        lv_timer_del(timer);
    }, 3000, mbox);
}

void UIManager::updateVolume(uint8_t volume) {
    // Update volume slider if it exists
    lv_obj_t* slider = lv_obj_get_child(radioScreen, 0);
    if (slider && lv_obj_check_type(slider, &lv_slider_class)) {
        lv_slider_set_value(slider, volume, LV_ANIM_OFF);
    }
}

void UIManager::updateBrightness(uint8_t brightness) {
    // Update display brightness (0-100% to 0-255)
    displayManager.setBrightness(map(brightness, 0, 100, 0, 255));
}

// Brightness changed callback
void UIManager::brightness_changed_cb(lv_event_t *e) {
    UIManager& ui = UIManager::getInstance();
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t brightness = lv_slider_get_value(slider);
    if (ui.brightnessCallback) {
        ui.brightnessCallback(brightness);
    }
}

void UIManager::initTheme() {
    // Initialize styles
    lv_style_init(&timeStyle);
    lv_style_set_text_font(&timeStyle, &lv_font_montserrat_48);
    lv_style_set_text_color(&timeStyle, lv_color_white());
    lv_style_set_text_align(&timeStyle, LV_TEXT_ALIGN_CENTER);
    
    lv_style_init(&dateStyle);
    lv_style_set_text_font(&dateStyle, &lv_font_montserrat_24);
    lv_style_set_text_color(&dateStyle, lv_color_hex(0xCCCCCC));
    lv_style_set_text_align(&dateStyle, LV_TEXT_ALIGN_CENTER);
    
    lv_style_init(&infoStyle);
    lv_style_set_text_font(&infoStyle, &lv_font_montserrat_20);
    lv_style_set_text_color(&infoStyle, lv_color_white());
    lv_style_set_text_align(&infoStyle, LV_TEXT_ALIGN_CENTER);
    
    lv_style_init(&buttonStyle);
    lv_style_set_radius(&buttonStyle, 10);
    lv_style_set_bg_opa(&buttonStyle, LV_OPA_30);
    lv_style_set_bg_color(&buttonStyle, lv_color_white());
    lv_style_set_border_width(&buttonStyle, 0);
    lv_style_set_pad_all(&buttonStyle, 10);
    
    lv_style_init(&buttonPressedStyle);
    lv_style_set_bg_opa(&buttonPressedStyle, LV_OPA_50);
    lv_style_set_bg_color(&buttonPressedStyle, lv_color_white());
    
    // Set the default screen color
    static lv_style_t screen_style;
    lv_style_init(&screen_style);
    lv_style_set_bg_color(&screen_style, lv_color_black());
    lv_style_set_bg_opa(&screen_style, LV_OPA_COVER);
    lv_style_set_pad_all(&screen_style, 0);
    lv_style_set_margin_all(&screen_style, 0);
    lv_style_set_border_width(&screen_style, 0);
    
    lv_obj_add_style(lv_scr_act(), &screen_style, 0);
    
    // Enable anti-aliasing for better text rendering
    lv_disp_set_antialiasing(lv_disp_get_default(), true);
    
    // Set default theme
    lv_theme_t* theme = lv_theme_default_init(
        lv_disp_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        darkTheme,  // Use dark theme
        &lv_font_montserrat_16
    );
    lv_disp_set_theme(lv_disp_get_default(), theme);
    lv_disp_set_theme(lv_disp_get_default(), theme);
}

void UIManager::createHomeScreen() {
    // Initialize screens if not already done
    if (!homeScreen) {
        homeScreen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(homeScreen, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_size(homeScreen, 800, 480);  // Use actual display dimensions
        
        // Create all UI elements
        timeLabel = lv_label_create(homeScreen);
        dateLabel = lv_label_create(homeScreen);
        tempLabel = lv_label_create(homeScreen);
        humidityLabel = lv_label_create(homeScreen);
        tvocLabel = lv_label_create(homeScreen);
        eco2Label = lv_label_create(homeScreen);
        nextAlarmLabel = lv_label_create(homeScreen);
        weatherIcon = lv_label_create(homeScreen);
        feelsLikeLabel = lv_label_create(homeScreen);
        
        // Initialize screens
        if (!settingsScreen) settingsScreen = lv_obj_create(NULL);
        if (!alarmScreen) alarmScreen = lv_obj_create(NULL);
        currentAlarmScreen = nullptr;
        
        // Time label
        lv_obj_add_style(timeLabel, &timeStyle, 0);
        lv_label_set_text(timeLabel, "--:--");
        lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 40);
        
        // Date label
        lv_obj_add_style(dateLabel, &dateStyle, 0);
        lv_label_set_text(dateLabel, "--.--.----");
        lv_obj_align_to(dateLabel, timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        
        // Weather info
        lv_obj_add_style(weatherIcon, &infoStyle, 0);
        lv_label_set_text(weatherIcon, "☀️");
        lv_obj_align(weatherIcon, LV_ALIGN_TOP_RIGHT, -20, 20);
        
        // Temperature
        lv_obj_add_style(tempLabel, &infoStyle, 0);
        lv_label_set_text(tempLabel, "--°C");
        lv_obj_align_to(tempLabel, weatherIcon, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
        
        // Air quality info
        lv_obj_add_style(humidityLabel, &infoStyle, 0);
        lv_label_set_text(humidityLabel, "Humidity: --%");
        lv_obj_align(humidityLabel, LV_ALIGN_BOTTOM_LEFT, 20, -20);
        
        lv_obj_add_style(tvocLabel, &infoStyle, 0);
        lv_label_set_text(tvocLabel, "TVOC: -- ppb");
        lv_obj_align_to(tvocLabel, humidityLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
        
        lv_obj_add_style(eco2Label, &infoStyle, 0);
        lv_label_set_text(eco2Label, "eCO2: -- ppm");
        lv_obj_align_to(eco2Label, tvocLabel, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
        
        // Next alarm
        lv_obj_add_style(nextAlarmLabel, &infoStyle, 0);
        lv_label_set_text(nextAlarmLabel, "Next alarm: None");
        lv_obj_align(nextAlarmLabel, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
        
        // Load the home screen
        lv_scr_load(homeScreen);
    }
}

void UIManager::createAlarmSettingsScreen() {
    if (alarmSettingsScreen) {
        lv_obj_del(alarmSettingsScreen);
    }
    
    alarmSettingsScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(alarmSettingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(alarmSettingsScreen, lv_color_black(), LV_PART_MAIN);
    
    // Back button
    lv_obj_t* btnBack = lv_btn_create(alarmSettingsScreen);
    lv_obj_add_style(btnBack, &buttonStyle, 0);
    lv_obj_add_style(btnBack, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnBack, 60, 40);
    lv_obj_align(btnBack, LV_ALIGN_TOP_LEFT, 10, 10);
    
    lv_obj_t* label = lv_label_create(btnBack);
    lv_label_set_text(label, LV_SYMBOL_LEFT);
    lv_obj_center(label);
    lv_obj_add_event_cb(btnBack, [](lv_event_t* e) {
        uiManager.showHomeScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Title
    lv_obj_t* title = lv_label_create(alarmSettingsScreen);
    lv_obj_add_style(title, &infoStyle, 0);
    lv_label_set_text(title, "Alarm Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Time picker container
    lv_obj_t* timeContainer = lv_obj_create(alarmSettingsScreen);
    lv_obj_remove_style_all(timeContainer);
    lv_obj_set_size(timeContainer, lv_pct(80), 120);
    lv_obj_align(timeContainer, LV_ALIGN_TOP_MID, 0, 60);
    
    // Hour roller
    static const char* hours[] = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11",
                                 "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", ""};
    lv_obj_t* hour_roller = lv_roller_create(timeContainer);
    lv_roller_set_options(hour_roller, 
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n"
        "12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23", 
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(hour_roller, 3);
    lv_obj_align(hour_roller, LV_ALIGN_LEFT_MID, 0, 0);
    
    // Colon
    lv_obj_t* colon = lv_label_create(timeContainer);
    lv_label_set_text(colon, ":");
    lv_obj_align(colon, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_32, 0);
    
    // Minute roller
    lv_obj_t* min_roller = lv_roller_create(timeContainer);
    lv_roller_set_options(min_roller, 
        "00\n05\n10\n15\n20\n25\n30\n35\n40\n45\n50\n55", 
        LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(min_roller, 3);
    lv_obj_align(min_roller, LV_ALIGN_RIGHT_MID, 0, 0);
    
    // Days of week container
    lv_obj_t* daysContainer = lv_obj_create(alarmSettingsScreen);
    lv_obj_remove_style_all(daysContainer);
    lv_obj_set_size(daysContainer, lv_pct(90), 60);
    lv_obj_align(daysContainer, LV_ALIGN_TOP_MID, 0, 200);
    
    // Day buttons
    const char* day_abbr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static bool days_selected[7] = {false};
    
    for (int i = 0; i < 7; i++) {
        lv_obj_t* btn = lv_btn_create(daysContainer);
        lv_obj_set_size(btn, 40, 40);
        lv_obj_align(btn, LV_ALIGN_LEFT_MID, i * 50, 0);
        lv_obj_add_style(btn, &buttonStyle, 0);
        lv_obj_add_style(btn, &buttonPressedStyle, LV_STATE_PRESSED);
        
        lv_obj_t* day_label = lv_label_create(btn);
        lv_label_set_text(day_label, day_abbr[i]);
        lv_obj_center(day_label);
        
        // Store day index in user data
        lv_obj_set_user_data(btn, (void*)(intptr_t)i);
        
        lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            lv_obj_t* btn = lv_event_get_target(e);
            int day = (int)(intptr_t)lv_obj_get_user_data(btn);
            days_selected[day] = !days_selected[day];
            
            if (days_selected[day]) {
                lv_obj_add_state(btn, LV_STATE_CHECKED);
            } else {
                lv_obj_clear_state(btn, LV_STATE_CHECKED);
            }
        }, LV_EVENT_CLICKED, NULL);
    }
    
    // Alarm toggle
    lv_obj_t* toggleContainer = lv_obj_create(alarmSettingsScreen);
    lv_obj_remove_style_all(toggleContainer);
    lv_obj_set_size(toggleContainer, lv_pct(80), 60);
    lv_obj_align(toggleContainer, LV_ALIGN_TOP_MID, 0, 280);
    
    lv_obj_t* toggleLabel = lv_label_create(toggleContainer);
    lv_label_set_text(toggleLabel, "Alarm Enabled");
    lv_obj_align(toggleLabel, LV_ALIGN_LEFT_MID, 0, 0);
    
    lv_obj_t* alarmToggle = lv_switch_create(toggleContainer);
    lv_obj_align(alarmToggle, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(alarmToggle, alarm_toggle_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Save button
    lv_obj_t* btnSave = lv_btn_create(alarmSettingsScreen);
    lv_obj_add_style(btnSave, &buttonStyle, 0);
    lv_obj_add_style(btnSave, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnSave, lv_pct(60), 50);
    lv_obj_align(btnSave, LV_ALIGN_BOTTOM_MID, 0, -20);
    
    lv_obj_t* saveLabel = lv_label_create(btnSave);
    lv_label_set_text(saveLabel, "Save Alarm");
    lv_obj_center(saveLabel);
    
    lv_obj_add_event_cb(btnSave, [](lv_event_t* e) {
        // Get the selected time
        lv_obj_t* hour_roller = lv_obj_get_child(lv_obj_get_child(alarmSettingsScreen, 2), 0);
        lv_obj_t* min_roller = lv_obj_get_child(lv_obj_get_child(alarmSettingsScreen, 2), 2);
        
        uint8_t hour = lv_roller_get_selected(hour_roller);
        uint8_t minute = lv_roller_get_selected(min_roller) * 5; // Convert from index to minutes (0,5,10...)
        
        if (uiManager.getAlarmCallback()) {
            uiManager.getAlarmCallback()(true, hour, minute, days_selected);
        }
        
        uiManager.showHomeScreen();
    }, LV_EVENT_CLICKED, NULL);
}
