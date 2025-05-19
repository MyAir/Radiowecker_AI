#include <Arduino.h>
#include <SPIFFS.h>
#include <lvgl.h>
#include "UIManager.h"
#include "DisplayManager.h"
#include "AudioManager.h"

// Include LVGL fonts
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_32);
LV_FONT_DECLARE(lv_font_montserrat_48);

// Static member is already initialized in the header file

// We'll use the static member functions declared in UIManager.h

bool UIManager::init() {
    // Initialize LVGL if not already done
    if (!lv_is_initialized()) {
        lv_init();
        
        // Initialize display and touch through DisplayManager
        if (!DisplayManager::getInstance().begin()) {
            Serial.println("Failed to initialize display manager");
            return false;
        }
        
        // Set initial brightness
        DisplayManager::getInstance().setBrightness(80);
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



void UIManager::updateVolume(uint8_t volume) {
    // Update volume slider if it exists
    lv_obj_t* slider = lv_obj_get_child(radioScreen, 0);
    if (slider && lv_obj_check_type(slider, &lv_slider_class)) {
        lv_slider_set_value(slider, volume, LV_ANIM_OFF);
    }
}

void UIManager::updateBrightness(uint8_t brightness) {
    // Update brightness using DisplayManager singleton
    DisplayManager::getInstance().setBrightness(brightness);
    
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

void UIManager::radio_volume_changed_cb(lv_event_t* e) {
    // Get the UIManager instance from user_data
    UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (!ui) return;
    
    // Access to the slider that triggered the event
    lv_obj_t* slider = lv_event_get_target(e);
    uint8_t volume = lv_slider_get_value(slider);
    
    // Call the volume callback if set
    if (ui->volumeCallback) {
        ui->volumeCallback(volume);
    }
}

void UIManager::brightness_changed_cb(lv_event_t* e) {
    // Get UIManager instance from user data
    UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (!ui) return;
    
    lv_obj_t* slider = lv_event_get_target(e);
    uint8_t brightness = lv_slider_get_value(slider);
    
    // Call the brightness callback if set
    if (ui->brightnessCallback) {
        ui->brightnessCallback(brightness);
    }
    
    // Also directly update brightness in DisplayManager
    DisplayManager::getInstance().setBrightness(brightness);
}

void UIManager::createRadioScreen() {
    radioScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(radioScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(radioScreen, lv_color_black(), LV_PART_MAIN);
    
    // Volume slider
    lv_obj_t* volumeSlider = lv_slider_create(radioScreen);
    lv_obj_set_size(volumeSlider, 200, 20);
    lv_obj_align(volumeSlider, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_slider_set_range(volumeSlider, 0, 100);
    lv_slider_set_value(volumeSlider, 70, LV_ANIM_OFF);
    // Use a static member function as callback instead of a capturing lambda
    lv_obj_add_event_cb(volumeSlider, radio_volume_changed_cb, LV_EVENT_VALUE_CHANGED, this);
}

// Static callback implementation
void UIManager::volume_changed_cb(lv_event_t* e) {
    // Get the UIManager instance from user_data
    UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (!ui) return;
    
    // Get slider value
    lv_obj_t* slider = lv_event_get_target(e);
    int32_t volume = lv_slider_get_value(slider);
    
    // Call volume callback if registered
    if (ui->volumeCallback) {
        ui->volumeCallback(volume);
    }
}

// Note: Removing erroneous continuation block

void UIManager::createSettingsScreen() {
    if (settingsScreen) {
        lv_obj_del(settingsScreen);
    }
    
    settingsScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(settingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(settingsScreen, lv_color_black(), LV_PART_MAIN);
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(settingsScreen);
    lv_obj_set_size(backBtn, 80, 40);
    lv_obj_align(backBtn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_t* backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, LV_SYMBOL_LEFT " Back");
    lv_obj_center(backLabel);
    // Use static member function as callback
    lv_obj_add_event_cb(backBtn, back_btn_clicked_cb, LV_EVENT_CLICKED, this);
}

// Static callback implementation
void UIManager::back_btn_clicked_cb(lv_event_t* e) {
    // Get the UIManager instance from user_data
    UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (!ui) return;
    
    // Call the showHomeScreen method on the UI instance
    ui->showHomeScreen();
    
    // Title
    lv_obj_t* titleLabel = lv_label_create(ui->settingsScreen);
    lv_label_set_text(titleLabel, "Settings");
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_24, 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 20);
    
    // Brightness control
    lv_obj_t* brightnessLabel = lv_label_create(ui->settingsScreen);
    lv_label_set_text(brightnessLabel, "Brightness");
    lv_obj_set_style_text_font(brightnessLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(brightnessLabel, LV_ALIGN_TOP_LEFT, 20, 80);
    
    lv_obj_t* brightnessSlider = lv_slider_create(ui->settingsScreen);
    lv_obj_set_size(brightnessSlider, 200, 20);
    lv_obj_align(brightnessSlider, LV_ALIGN_TOP_LEFT, 20, 110);
    lv_slider_set_range(brightnessSlider, 10, 255);
    // Set default brightness to 80%
    lv_slider_set_value(brightnessSlider, 200, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightnessSlider, brightness_changed_cb, LV_EVENT_VALUE_CHANGED, ui);
    
    // Volume control
    lv_obj_t* volumeLabel = lv_label_create(ui->settingsScreen);
    lv_label_set_text(volumeLabel, "Volume");
    lv_obj_set_style_text_font(volumeLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(volumeLabel, LV_ALIGN_TOP_LEFT, 20, 160);
    
    lv_obj_t* volumeSlider = lv_slider_create(ui->settingsScreen);
    lv_obj_set_size(volumeSlider, 200, 20);
    lv_obj_align(volumeSlider, LV_ALIGN_TOP_LEFT, 20, 190);
    lv_slider_set_range(volumeSlider, 0, 100);
    lv_slider_set_value(volumeSlider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(volumeSlider, volume_changed_cb, LV_EVENT_VALUE_CHANGED, ui);
    
    // Theme toggle
    lv_obj_t* themeLabel = lv_label_create(ui->settingsScreen);
    lv_label_set_text(themeLabel, "Dark Theme");
    lv_obj_set_style_text_font(themeLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(themeLabel, LV_ALIGN_TOP_LEFT, 20, 240);
    
    lv_obj_t* themeSwitch = lv_switch_create(ui->settingsScreen);
    lv_obj_set_size(themeSwitch, 60, 30);
    lv_obj_align(themeSwitch, LV_ALIGN_TOP_RIGHT, -20, 240);
    if (ui->darkTheme) {
        lv_obj_add_state(themeSwitch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(themeSwitch, theme_switch_cb, LV_EVENT_VALUE_CHANGED, ui);
}

// Static callback implementation for theme switch
void UIManager::theme_switch_cb(lv_event_t* e) {
    UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (!ui) return;
    
    lv_obj_t* sw = lv_event_get_target(e);
    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);
    
    // Toggle dark theme
    ui->darkTheme = checked;
    
    // Apply theme change
    ui->initTheme();
}

void UIManager::showScreen(lv_obj_t* screen) {
    if (currentAlarmScreen) {
        lv_obj_del(currentAlarmScreen);
        currentAlarmScreen = nullptr;
    }
    if (screen) {
        currentAlarmScreen = screen;
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
    lv_scr_load_anim(alarmSettingsScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    currentScreen = alarmSettingsScreen;
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

void UIManager::initTheme() {
    // Initialize styles
    lv_style_init(&timeStyle);
    lv_style_set_text_font(&timeStyle, &lv_font_montserrat_48);
    lv_style_set_text_color(&timeStyle, lv_color_white());
    
    lv_style_init(&dateStyle);
    lv_style_set_text_font(&dateStyle, &lv_font_montserrat_24);
    lv_style_set_text_color(&dateStyle, lv_color_white());
    
    lv_style_init(&infoStyle);
    lv_style_set_text_font(&infoStyle, &lv_font_montserrat_16);
    lv_style_set_text_color(&infoStyle, lv_color_white());
    
    lv_style_init(&buttonStyle);
    lv_style_set_radius(&buttonStyle, 10);
    lv_style_set_bg_color(&buttonStyle, lv_color_hex(0x4CAF50));
    lv_style_set_bg_opa(&buttonStyle, LV_OPA_COVER);
    lv_style_set_pad_all(&buttonStyle, 10);
    
    lv_style_init(&buttonPressedStyle);
    lv_style_set_bg_color(&buttonPressedStyle, lv_color_hex(0x388E3C));
    
    // Set the default screen color
    static lv_style_t screen_style;
    lv_style_init(&screen_style);
    lv_style_set_pad_all(&screen_style, 0);
    lv_style_set_bg_color(&screen_style, lv_color_black());
    lv_style_set_text_color(&screen_style, lv_color_white());
    lv_style_set_border_width(&screen_style, 0);
    
    lv_obj_add_style(lv_scr_act(), &screen_style, 0);
    
    // Note: Anti-aliasing is enabled by default in LVGL
    
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

void UIManager::hideAlarmScreen() {
    if (currentAlarmScreen) {
        lv_obj_del(currentAlarmScreen);
        currentAlarmScreen = nullptr;
    }
    // Also stop any playing alarm sound
    if (AudioManager::getInstance().isPlaying()) {
        AudioManager::getInstance().stop();
    }
    // Return to home screen
    showHomeScreen();
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
    lv_obj_add_event_cb(btnBack, back_btn_clicked_cb, LV_EVENT_CLICKED, this);
    
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
        
        // Store UIManager instance and day index in user_data struct
        UserData* data = new UserData();
        data->ui = this;
        data->value = i;  // Store day index in value field
        
        lv_obj_add_event_cb(btn, day_btn_clicked_cb, LV_EVENT_CLICKED, data);
    }
    
    // Alarm toggle container
    lv_obj_t* toggleContainer = lv_obj_create(alarmSettingsScreen);
    lv_obj_remove_style_all(toggleContainer);
    lv_obj_set_size(toggleContainer, lv_pct(80), 60);
    lv_obj_align(toggleContainer, LV_ALIGN_TOP_MID, 0, 280);

    lv_obj_t* toggleLabel = lv_label_create(toggleContainer);
    lv_label_set_text(toggleLabel, "Alarm Enabled");
    lv_obj_align(toggleLabel, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* alarmToggle = lv_switch_create(toggleContainer);
    lv_obj_align(alarmToggle, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(alarmToggle, alarm_toggle_cb, LV_EVENT_VALUE_CHANGED, this);

    // Save button
    lv_obj_t* btnSave = lv_btn_create(alarmSettingsScreen);
    lv_obj_add_style(btnSave, &buttonStyle, 0);
    lv_obj_add_style(btnSave, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnSave, lv_pct(60), 50);
    lv_obj_align(btnSave, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* saveLabel = lv_label_create(btnSave);
    lv_label_set_text(saveLabel, "Save Alarm");
    lv_obj_center(saveLabel);

    lv_obj_add_event_cb(btnSave, save_alarm_cb, LV_EVENT_CLICKED, this);
}

// Static callback implementation for day buttons
void UIManager::day_btn_clicked_cb(lv_event_t* e) {
    // Get the user data containing UIManager instance and day index
    UserData* data = static_cast<UserData*>(lv_event_get_user_data(e));
    if (!data) return;
    
    // Get button and day index
    lv_obj_t* btn = lv_event_get_target(e);
    int day = data->value; // day index is stored in value field
    
    // Update selection state (using a static array to track button states)
    static bool days_selected[7] = {false};
    days_selected[day] = !days_selected[day];
    
    // Toggle button appearance
    if (days_selected[day]) {
        lv_obj_add_state(btn, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
    }
}

// Static callback for alarm toggle
void UIManager::alarm_toggle_cb(lv_event_t* e) {
    UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (!ui) return;
    
    lv_obj_t* sw = lv_event_get_target(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    
    // This would require additional implementation to actually do something
    // with the alarm toggle state
}

// Static callback implementation
void UIManager::save_alarm_cb(lv_event_t* e) {
    UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (!ui) return;
    
    // Get the selected time
    lv_obj_t* timeContainer = lv_obj_get_child(ui->alarmSettingsScreen, 2);
    if (!timeContainer) return;
    
    lv_obj_t* hour_roller = lv_obj_get_child(timeContainer, 0);
    lv_obj_t* min_roller = lv_obj_get_child(timeContainer, 2);
    if (!hour_roller || !min_roller) return;
    
    uint8_t hour = lv_roller_get_selected(hour_roller);
    uint8_t minute = lv_roller_get_selected(min_roller) * 5; // Convert from index to minutes (0,5,10...)
    
    bool days[7] = {false};
    // Get the selected days from the buttons
    lv_obj_t* daysContainer = lv_obj_get_child(ui->alarmSettingsScreen, 3);
    if (daysContainer) {
        for (int i = 0; i < 7; i++) {
            lv_obj_t* btn = lv_obj_get_child(daysContainer, i);
            if (btn) {
                days[i] = lv_obj_has_state(btn, LV_STATE_CHECKED);
            }
        }
    }
    
    // Trigger the alarm callback
    if (ui->alarmCallback) {
        ui->alarmCallback(true, hour, minute, days);
    }
    
    // Return to home screen
    ui->showHomeScreen();
}
