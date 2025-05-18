#include "UIManager.h"
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Arduino.h>

extern TFT_eSPI tft;

void UIManager::begin() {
    // Initialize display
    tft.begin();
    tft.setRotation(1);  // Adjust rotation as needed
    tft.fillScreen(TFT_BLACK);
    
    // Initialize LVGL
    lv_init();
    
    // Initialize display buffer
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf1[TFT_WIDTH * 10];  // Adjust buffer size as needed
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, TFT_WIDTH * 10);
    
    // Initialize the display
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TFT_WIDTH;
    disp_drv.ver_res = TFT_HEIGHT;
    disp_drv.flush_cb = [](lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
        uint32_t w = (area->x2 - area->x1 + 1);
        uint32_t h = (area->y2 - area->y1 + 1);
        tft.startWrite();
        tft.setAddrWindow(area->x1, area->y1, w, h);
        tft.pushColors((uint16_t *)&color_p->full, w * h, true);
        tft.endWrite();
        lv_disp_flush_ready(disp);
    };
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Initialize touch
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = [](lv_indev_drv_t *drv, lv_indev_data_t *data) {
        uint16_t touchX, touchY;
        bool touched = tft.getTouch(&touchX, &touchY);
        if (touched) {
            data->point.x = touchX;
            data->point.y = touchY;
            data->state = LV_INDEV_STATE_PRESSED;
        } else {
            data->state = LV_INDEV_STATE_RELEASED;
        }
    };
    lv_indev_drv_register(&indev_drv);
    
    // Initialize theme
    initTheme();
    
    // Create screens
    createHomeScreen();
    createAlarmSettingsScreen();
    createRadioScreen();
    createSettingsScreen();
    
    // Show home screen by default
    showHomeScreen();
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

void UIManager::initTheme() {
    // Initialize styles
    lv_style_init(&timeStyle);
    lv_style_set_text_font(&timeStyle, &lv_font_montserrat_48);
    lv_style_set_text_color(&timeStyle, lv_color_white());
    
    lv_style_init(&dateStyle);
    lv_style_set_text_font(&dateStyle, &lv_font_montserrat_24);
    lv_style_set_text_color(&dateStyle, lv_color_white());
    
    lv_style_init(&infoStyle);
    lv_style_set_text_font(&infoStyle, &lv_font_montserrat_20);
    lv_style_set_text_color(&infoStyle, lv_color_white());
    
    lv_style_init(&buttonStyle);
    lv_style_set_radius(&buttonStyle, 10);
    lv_style_set_bg_opa(&buttonStyle, LV_OPA_30);
    lv_style_set_bg_color(&buttonStyle, lv_color_white());
    lv_style_set_border_width(&buttonStyle, 0);
    
    lv_style_init(&buttonPressedStyle);
    lv_style_set_bg_opa(&buttonPressedStyle, LV_OPA_50);
}

void UIManager::createHomeScreen() {
    homeScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(homeScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(homeScreen, lv_color_black(), LV_PART_MAIN);
    
    // Time label
    timeLabel = lv_label_create(homeScreen);
    lv_obj_add_style(timeLabel, &timeStyle, 0);
    lv_label_set_text(timeLabel, "--:--");
    lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 40);
    
    // Date label
    dateLabel = lv_label_create(homeScreen);
    lv_obj_add_style(dateLabel, &dateStyle, 0);
    lv_label_set_text(dateLabel, "--.--.----");
    lv_obj_align_to(dateLabel, timeLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    
    // Weather info
    weatherIcon = lv_label_create(homeScreen);
    lv_obj_add_style(weatherIcon, &infoStyle, 0);
    lv_label_set_text(weatherIcon, "☀️");
    lv_obj_align(weatherIcon, LV_ALIGN_TOP_RIGHT, -20, 20);
    
    tempLabel = lv_label_create(homeScreen);
    lv_obj_add_style(tempLabel, &infoStyle, 0);
    lv_label_set_text(tempLabel, "--°C");
    lv_obj_align_to(tempLabel, weatherIcon, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    // Air quality info
    tvocLabel = lv_label_create(homeScreen);
    lv_obj_add_style(tvocLabel, &infoStyle, 0);
    lv_label_set_text(tvocLabel, "TVOC: -- ppb");
    lv_obj_align(tvocLabel, LV_ALIGN_BOTTOM_LEFT, 20, -80);
    
    eco2Label = lv_label_create(homeScreen);
    lv_obj_add_style(eco2Label, &infoStyle, 0);
    lv_label_set_text(eco2Label, "eCO2: -- ppm");
    lv_obj_align_to(eco2Label, tvocLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    humidityLabel = lv_label_create(homeScreen);
    lv_obj_add_style(humidityLabel, &infoStyle, 0);
    lv_label_set_text(humidityLabel, "Humidity: --%");
    lv_obj_align_to(humidityLabel, eco2Label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    // Next alarm
    nextAlarmLabel = lv_label_create(homeScreen);
    lv_obj_add_style(nextAlarmLabel, &infoStyle, 0);
    lv_label_set_text(nextAlarmLabel, "Alarm: --:--");
    lv_obj_align(nextAlarmLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void UIManager::createAlarmSettingsScreen() {
    alarmSettingsScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(alarmSettingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(alarmSettingsScreen, lv_color_black(), LV_PART_MAIN);
    
    // Back button
    lv_obj_t* btnBack = lv_btn_create(alarmSettingsScreen);
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
    lv_obj_t* title = lv_label_create(alarmSettingsScreen);
    lv_obj_add_style(title, &infoStyle, 0);
    lv_label_set_text(title, "Alarm Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // Time picker
    lv_obj_t* timePicker = lv_roller_create(alarmSettingsScreen);
    lv_roller_set_options(timePicker, "00:00\n01:00\n02:00\n03:00\n04:00\n05:00\n06:00\n07:00\n08:00\n09:00\n10:00\n11:00\n12:00\n13:00\n14:00\n15:00\n16:00\n17:00\n18:00\n19:00\n20:00\n21:00\n22:00\n23:00", LV_ROLLER_MODE_NORMAL);
    lv_obj_align(timePicker, LV_ALIGN_TOP_MID, 0, 70);
    lv_roller_set_visible_row_count(timePicker, 3);
    
    // Day buttons
    const char* days[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (int i = 0; i < 7; i++) {
        lv_obj_t* btn = lv_btn_create(alarmSettingsScreen);
        lv_obj_add_style(btn, &buttonStyle, 0);
        lv_obj_add_style(btn, &buttonPressedStyle, LV_STATE_PRESSED);
        lv_obj_set_size(btn, 40, 40);
        lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 50 + i * 45, 180);
        
        label = lv_label_create(btn);
        lv_label_set_text(label, days[i]);
        lv_obj_center(label);
    }
    
    // Save button
    lv_obj_t* btnSave = lv_btn_create(alarmSettingsScreen);
    lv_obj_add_style(btnSave, &buttonStyle, 0);
    lv_obj_add_style(btnSave, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnSave, 200, 50);
    lv_obj_align(btnSave, LV_ALIGN_BOTTOM_MID, 0, -30);
    
    label = lv_label_create(btnSave);
    lv_label_set_text(label, "Save Alarm");
    lv_obj_center(label);
}
