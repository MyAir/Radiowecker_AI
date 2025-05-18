    lv_obj_align(volumeSlider, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_slider_set_range(volumeSlider, 0, 100);
    lv_slider_set_value(volumeSlider, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(volumeSlider, volume_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
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
    lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
}

void UIManager::showHomeScreen() {
    showScreen(homeScreen);
}

void UIManager::showAlarmSettingsScreen() {
    showScreen(alarmSettingsScreen);
}

void UIManager::showRadioScreen() {
    showScreen(radioScreen);
}

void UIManager::showSettingsScreen() {
    showScreen(settingsScreen);
}

void UIManager::updateTime() {
    static char timeStr[6];
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    lv_label_set_text(timeLabel, timeStr);
    
    static char dateStr[32];
    strftime(dateStr, sizeof(dateStr), "%A, %d.%m.%Y", &timeinfo);
    lv_label_set_text(dateLabel, dateStr);
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
    lv_obj_t* timer = lv_timer_create([](lv_timer_t* timer) {
        lv_obj_t* mbox = (lv_obj_t*)timer->user_data;
        lv_msgbox_close(mbox);
        lv_timer_del(timer);
    }, 3000, mbox);
}

void UIManager::updateVolume(uint8_t volume) {
    // Update volume indicator if on radio screen
    // This would be called when the volume changes via hardware buttons or UI
}

void UIManager::updateBrightness(uint8_t brightness) {
    // Update display brightness
    ledcWrite(0, map(brightness, 0, 100, 0, 255));
}
