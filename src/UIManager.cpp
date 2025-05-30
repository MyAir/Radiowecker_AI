#include <Arduino.h>
#include <SPIFFS.h>
#include <lvgl.h>
#include "UIManager.h"
#include "DisplayManager.h"
#include "AudioManager.h"

// Include LVGL standard fonts
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_32);
LV_FONT_DECLARE(lv_font_montserrat_48);

// Include custom Montserrat fonts with German umlaut support
LV_FONT_DECLARE(lv_font_montserrat_16x);
LV_FONT_DECLARE(lv_font_montserrat_20x);
LV_FONT_DECLARE(lv_font_montserrat_40x);

// Include weather icons
LV_IMG_DECLARE(icon_02d);

// Define the static instance pointer
UIManager* UIManager::instance = nullptr;

// We'll use the static member functions declared in UIManager.h

bool UIManager::init() {
    // Get reference to DisplayManager
    DisplayManager& dm = DisplayManager::getInstance();
    
    // Verify that LVGL is properly initialized
    if (lv_disp_get_default() == NULL) {
        Serial.println("[ERROR] LVGL display not initialized! Cannot create UI.");
        return false;
    }
    
    Serial.println("UIManager initializing UI elements");
    
    // Clear the screen to ensure we're working with a clean canvas
    lv_obj_clean(lv_scr_act());
    
    // Initialize styles first
    initTheme();
    
    // Create all screens
    createHomeScreen();
    createAlarmSettingsScreen();
    createRadioScreen();
    createSettingsScreen();
    
    // Show the home screen with a simple animation
    showHomeScreen();
    
    // Force an immediate refresh to make sure elements are visible
    lv_refr_now(lv_disp_get_default());
    
    Serial.println("UIManager initialization complete");
    return true;
}

void UIManager::showMainScreen() {
    showHomeScreen();
}

void UIManager::updateTime(const char* timeStr) {
    static uint32_t lastUpdate = 0;
    uint32_t now = millis();
    
    if (!timeLabel) {
        static uint32_t lastErrorLog = 0;
        if (now - lastErrorLog > 5000) {  // Log error max every 5 seconds
            Serial.println("[ERROR] Time label is null!");
            lastErrorLog = now;
        }
        return;
    }
    
    if (!timeStr) {
        Serial.println("[WARNING] Time string is null!");
        return;
    }
    
    // Only update if the time has actually changed and at least 100ms has passed
    const char* currentText = lv_label_get_text(timeLabel);
    if (currentText && strcmp(currentText, timeStr) == 0) {
        return;
    }
    
    // Debug output
    // if (now - lastUpdate >= 1000) {  // Log at most once per second
    //     Serial.printf("[DEBUG] Updating time from '%s' to '%s'\n", 
    //                  currentText ? currentText : "NULL", timeStr);
    //     lastUpdate = now;
    // }
    
    // Update the label text
    lv_label_set_text(timeLabel, timeStr);
    
    // Invalidate the time label
    lv_obj_invalidate(timeLabel);
    
    // Always do a refresh to ensure updates work properly
    // This is more reliable, though it may have minimal flicker
    lv_refr_now(lv_disp_get_default());
    
    // Update debug tracking
    if (now - lastUpdate >= 1000) {
        lastUpdate = now;
    }
}

void UIManager::updateDate(const char* dateStr) {
    if (!dateLabel) {
        Serial.println("[ERROR] Date label is null!");
        return;
    }
    
    if (!dateStr) {
        Serial.println("[WARNING] Date string is null!");
        return;
    }
    
    // Make sure the date string isn't too long and has proper formatting
    char cleanDateStr[32];
    strncpy(cleanDateStr, dateStr, sizeof(cleanDateStr) - 1);
    cleanDateStr[sizeof(cleanDateStr) - 1] = '\0'; // Ensure null termination
    
    // Update the label text
    lv_label_set_text(dateLabel, cleanDateStr);
    
    // Invalidate the date label and refresh
    lv_obj_invalidate(dateLabel);
    lv_refr_now(lv_disp_get_default());
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
        
        // Set color based on temperature range
        lv_color_t temp_color;
        if (temp < 16) {
            // Too cold - Blue
            temp_color = lv_color_hex(0x00AFFF); // Light blue for better visibility
        } else if (temp <= 23) {
            // Comfortable - Green
            temp_color = lv_color_hex(0x00FF00);
        } else if (temp <= 26) {
            // Warm - Orange
            temp_color = lv_color_hex(0xFF9A00);
        } else {
            // Too hot - Red
            temp_color = lv_color_hex(0xFF0000);
        }
        
        lv_obj_set_style_text_color(tempLabel, temp_color, 0);
        
        Serial.printf("[DEBUG] UIManager::updateTemperature(%.1f°C) - Color set based on threshold\n", temp);
        
        // Invalidate the label and its parent to ensure proper redraw
        // Only invalidate the label itself, not the parent
        lv_obj_invalidate(tempLabel);
    } else {
        Serial.println("[WARNING] tempLabel is null in updateTemperature");
    }
}

void UIManager::updateHumidity(float humidity) {
    if (humidityLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f%%", humidity);
        lv_label_set_text(humidityLabel, buf);
        
        // Set color based on humidity level
        lv_color_t humidity_color;
        if (humidity < 40) {
            // Too dry - Yellow
            humidity_color = lv_color_hex(0xFFD700); // Gold yellow
        } else if (humidity <= 60) {
            // Optimal - Green
            humidity_color = lv_color_hex(0x00FF00);
        } else {
            // Too humid - Blue
            humidity_color = lv_color_hex(0x00AFFF); // Light blue, better visibility
        }
        
        lv_obj_set_style_text_color(humidityLabel, humidity_color, 0);
        
        Serial.printf("[DEBUG] UIManager::updateHumidity(%.0f%%) - Color set based on threshold\n", humidity);
        
        // Invalidate the label and its parent to ensure proper redraw
        // Only invalidate the label itself, not the parent
        lv_obj_invalidate(humidityLabel);
    } else {
        Serial.println("[WARNING] humidityLabel is null in updateHumidity");
    }
}

void UIManager::updateTVOC(uint16_t tvoc) {
    if (tvocLabel) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u ppb", tvoc);
        lv_label_set_text(tvocLabel, buf);
        
        // Set color based on TVOC concentration
        lv_color_t tvoc_color;
        if (tvoc < 400) {
            // Good - Green
            tvoc_color = lv_color_hex(0x00FF00);
        } else if (tvoc < 1200) {
            // Moderate - Orange
            tvoc_color = lv_color_hex(0xFF9A00);
        } else if (tvoc < 4000) {
            // Bad - Red
            tvoc_color = lv_color_hex(0xFF0000);
        } else {
            // Very Bad - Dark Red
            tvoc_color = lv_color_hex(0xAA0000);
        }
        
        lv_obj_set_style_text_color(tvocLabel, tvoc_color, 0);
        
        Serial.printf("[DEBUG] UIManager::updateTVOC(%u ppb) - Color set based on threshold\n", tvoc);
        
        // Invalidate the label and its parent to ensure proper redraw
        // Only invalidate the label itself, not the parent
        lv_obj_invalidate(tvocLabel);
    } else {
        Serial.println("[WARNING] tvocLabel is null in updateTVOC");
    }
}

void UIManager::updateCO2(uint16_t eco2) {
    if (eco2Label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u ppm", eco2);
        lv_label_set_text(eco2Label, buf);
        
        // Set color based on CO2 concentration
        lv_color_t co2_color;
        if (eco2 < 1000) {
            // Good - Green
            co2_color = lv_color_hex(0x00FF00);
        } else if (eco2 <= 2000) {
            // Warning - Orange
            co2_color = lv_color_hex(0xFF9A00);
        } else {
            // Danger - Red
            co2_color = lv_color_hex(0xFF0000);
        }
        
        lv_obj_set_style_text_color(eco2Label, co2_color, 0);
        
        Serial.printf("[DEBUG] UIManager::updateCO2(%u ppm) - Color set based on threshold\n", eco2);
        
        // Invalidate the label and its parent to ensure proper redraw
        // Only invalidate the label itself, not the parent
        lv_obj_invalidate(eco2Label);
    } else {
        Serial.println("[WARNING] eco2Label is null in updateCO2");
    }
}

void UIManager::updateNextAlarm(uint8_t hour, uint8_t minute, bool enabled) {
    if (nextAlarmLabel) {
        char buf[32];
        if (enabled) {
            snprintf(buf, sizeof(buf), "Next: %02d:%02d", hour, minute);
        } else {
            snprintf(buf, sizeof(buf), "No Alarms");
        }
        lv_label_set_text(nextAlarmLabel, buf);
    }
}

void UIManager::updateWifiSsid(const char* ssid) {
    if (wifiLabel) {
        char buf[64];
        snprintf(buf, sizeof(buf), "WiFi: %s", ssid ? ssid : "---");
        lv_label_set_text(wifiLabel, buf);
        
        // Refresh the label
        lv_obj_invalidate(wifiLabel);
    }
}

void UIManager::updateIpAddress(const char* ipAddress) {
    if (ipLabel) {
        char buf[64];
        snprintf(buf, sizeof(buf), "IP: %s", ipAddress ? ipAddress : "---");
        lv_label_set_text(ipLabel, buf);
        
        // Refresh the label
        lv_obj_invalidate(ipLabel);
    }
}

void UIManager::updateWifiQuality(int quality) {
    if (!wifiQualityLabel) {
        // If label doesn't exist, create it or handle the error
        Serial.println("[ERROR] WiFi quality label is null!");
        return;
    }
    
    // Format the signal quality string with an emoji representation
    char qualityStr[20];
    if (quality >= 80) {
        snprintf(qualityStr, sizeof(qualityStr), LV_SYMBOL_WIFI " %d%%", quality);
    } else if (quality >= 60) {
        snprintf(qualityStr, sizeof(qualityStr), LV_SYMBOL_WIFI " %d%%", quality);
    } else if (quality >= 40) {
        snprintf(qualityStr, sizeof(qualityStr), LV_SYMBOL_WIFI " %d%%", quality);
    } else {
        snprintf(qualityStr, sizeof(qualityStr), LV_SYMBOL_WIFI " %d%%", quality);
    }
    
    if (wifiQualityLabel) {
        lv_label_set_text(wifiQualityLabel, qualityStr);
    }
}

void UIManager::updateCurrentWeather(float temp, float feels_like, const char* description, const char* iconCode) {
    if (!currentTempLabel || !feelsLikeLabel || !weatherDescLabel || !weatherIcon) {
        Serial.println("[ERROR] Weather UI elements not initialized!");
        return;
    }
    
    // Update temperature
    if (currentTempLabel) {
        char tempStr[16];
        snprintf(tempStr, sizeof(tempStr), "%.1f°C", temp);
        lv_label_set_text(currentTempLabel, tempStr);
    }
    
    // Update feels like temperature
    if (feelsLikeLabel) {
        char feelsLikeStr[32];
        snprintf(feelsLikeStr, sizeof(feelsLikeStr), "Gefühlt: %.1f°C", feels_like);
        lv_label_set_text(feelsLikeLabel, feelsLikeStr);
    }
    
    // Update description
    if (weatherDescLabel && description) {
        lv_label_set_text(weatherDescLabel, description);
    }
    
    // Update weather icon
    if (iconCode) {
        // Remove the old image if it exists
        if (weatherIconImg) {
            lv_obj_del(weatherIconImg);
            weatherIconImg = nullptr;
        }
        
        // Hide the text icon if it exists
        if (weatherIcon) {
            lv_obj_add_flag(weatherIcon, LV_OBJ_FLAG_HIDDEN);
        }
        
        // Get the parent of the weather icon
        lv_obj_t* parent = lv_obj_get_parent(weatherIcon);
        if (!parent) {
            parent = lv_scr_act();
        }
        
        // Create a new weather icon with proper transparency handling
        weatherIconImg = create_weather_icon(parent, iconCode);
        if (weatherIconImg) {
            // Position the icon where the text icon was
            lv_obj_align_to(weatherIconImg, weatherIcon, LV_ALIGN_CENTER, 0, 0);
            lv_obj_clear_flag(weatherIconImg, LV_OBJ_FLAG_HIDDEN);
            
            // Ensure the parent has no background
            lv_obj_set_style_bg_opa(parent, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_opa(parent, LV_OPA_TRANSP, 0);
            lv_obj_set_style_outline_opa(parent, LV_OPA_TRANSP, 0);
            lv_obj_set_style_pad_all(parent, 0, 0);
        } else {
            // Fall back to using a direct image object
            if (weatherIcon) {
                // Hide text label
                lv_obj_add_flag(weatherIcon, LV_OBJ_FLAG_HIDDEN);
                
                // Create image if needed
                if (weatherIconImg == nullptr) {
                    weatherIconImg = lv_img_create(lv_obj_get_parent(weatherIcon));
                    lv_img_set_src(weatherIconImg, &icon_02d);
                    lv_obj_align_to(weatherIconImg, weatherIcon, LV_ALIGN_CENTER, 0, 0);
                }
                lv_obj_clear_flag(weatherIconImg, LV_OBJ_FLAG_HIDDEN);
            }
        }
    } else if (weatherIcon) {
        // No icon code provided, use the default icon_02d image
        // Hide text label
        lv_obj_add_flag(weatherIcon, LV_OBJ_FLAG_HIDDEN);
        
        // Create or show image
        if (weatherIconImg == nullptr) {
            weatherIconImg = lv_img_create(lv_obj_get_parent(weatherIcon));
            lv_img_set_src(weatherIconImg, &icon_02d);
            lv_obj_align_to(weatherIconImg, weatherIcon, LV_ALIGN_CENTER, 0, 0);
        }
        lv_obj_clear_flag(weatherIconImg, LV_OBJ_FLAG_HIDDEN);
    }
    
    Serial.printf("[INFO] Updated current weather: %.1f°C, Gefühlt: %.1f°C, %s\n", 
                 temp, feels_like, description);
                 
    // Force refresh
    lv_obj_invalidate(weatherPanel);
}

void UIManager::updateMorningForecast(float temp, float pop, const char* iconCode) {
    if (!morningTempLabel || !morningRainLabel || !morningIcon) {
        Serial.println("[ERROR] Morning forecast UI elements not initialized!");
        return;
    }
    
    // Update temperature
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f°C", temp);
    if (morningTempLabel) {
        lv_label_set_text(morningTempLabel, tempStr);
    }
    
    // Update rain probability
    char rainStr[16];
    snprintf(rainStr, sizeof(rainStr), "Regen: %.0f%%", pop * 100);
    if (morningRainLabel) {
        lv_label_set_text(morningRainLabel, rainStr);
    }
    
    // Update weather icon
    if (iconCode) {
        // Remove the old image if it exists
        if (morningIconImg) {
            lv_obj_del(morningIconImg);
            morningIconImg = nullptr;
        }
        
        // Hide the text icon if it exists
        if (morningIcon) {
            lv_obj_add_flag(morningIcon, LV_OBJ_FLAG_HIDDEN);
        }
        
        // Remove old icon if it exists
        if (morningIconImg) {
            lv_obj_del(morningIconImg);
            morningIconImg = nullptr;
        }
        
        // Hide the text icon
        if (morningIcon) {
            lv_obj_add_flag(morningIcon, LV_OBJ_FLAG_HIDDEN);
        }
        
        // Create a new weather icon
        morningIconImg = create_weather_icon(lv_scr_act(), iconCode);
        if (morningIconImg) {
            // Position and size the icon
            lv_obj_set_size(morningIconImg, 40, 40);
            lv_obj_align_to(morningIconImg, morningIcon, LV_ALIGN_CENTER, 0, 0);
            lv_obj_clear_flag(morningIconImg, LV_OBJ_FLAG_HIDDEN);
            
            // Make sure the parent container is transparent
            lv_obj_set_style_bg_opa(lv_obj_get_parent(morningIconImg), LV_OPA_TRANSP, 0);
        } else {
            // Fall back to using the icon_02d image
            if (morningIcon) {
                // Hide text label
                lv_obj_add_flag(morningIcon, LV_OBJ_FLAG_HIDDEN);
                
                // Create image if needed
                if (morningIconImg == nullptr) {
                    morningIconImg = lv_img_create(lv_obj_get_parent(morningIcon));
                    lv_img_set_src(morningIconImg, &icon_02d);
                    lv_obj_align_to(morningIconImg, morningIcon, LV_ALIGN_CENTER, 0, 0);
                }
                lv_obj_clear_flag(morningIconImg, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    
    Serial.printf("[INFO] Updated morning forecast: %.1f°C, Rain: %.0f%%\n", temp, pop * 100);
}

void UIManager::updateAfternoonForecast(float temp, float pop, const char* iconCode) {
    if (!afternoonTempLabel || !afternoonRainLabel || !afternoonIcon) {
        Serial.println("[ERROR] Afternoon forecast UI elements not initialized!");
        return;
    }
    
    // Update temperature
    char tempStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.1f°C", temp);
    lv_label_set_text(afternoonTempLabel, tempStr);
    
    // Update rain probability
    char rainStr[16];
    snprintf(rainStr, sizeof(rainStr), "Regen: %.0f%%", pop * 100);
    lv_label_set_text(afternoonRainLabel, rainStr);
    
    // Update weather icon
    if (iconCode) {
        // Remove the old image if it exists
        if (afternoonIconImg) {
            lv_obj_del(afternoonIconImg);
            afternoonIconImg = nullptr;
        }
        
        // Hide the text icon if it exists
        if (afternoonIcon) {
            lv_obj_add_flag(afternoonIcon, LV_OBJ_FLAG_HIDDEN);
        }
        
        // Remove old icon if it exists
        if (afternoonIconImg) {
            lv_obj_del(afternoonIconImg);
            afternoonIconImg = nullptr;
        }
        
        // Hide the text icon
        if (afternoonIcon) {
            lv_obj_add_flag(afternoonIcon, LV_OBJ_FLAG_HIDDEN);
        }
        
        // Create a new weather icon
        afternoonIconImg = create_weather_icon(lv_scr_act(), iconCode);
        if (afternoonIconImg) {
            // Position and size the icon
            lv_obj_set_size(afternoonIconImg, 40, 40);
            lv_obj_align_to(afternoonIconImg, afternoonIcon, LV_ALIGN_CENTER, 0, 0);
            lv_obj_clear_flag(afternoonIconImg, LV_OBJ_FLAG_HIDDEN);
            
            // Make sure the parent container is transparent
            lv_obj_set_style_bg_opa(lv_obj_get_parent(afternoonIconImg), LV_OPA_TRANSP, 0);
        } else {
            // Fall back to using the icon_02d image
            if (afternoonIcon) {
                // Hide text label
                lv_obj_add_flag(afternoonIcon, LV_OBJ_FLAG_HIDDEN);
                
                // Create image if needed
                if (afternoonIconImg == nullptr) {
                    afternoonIconImg = lv_img_create(lv_obj_get_parent(afternoonIcon));
                    lv_img_set_src(afternoonIconImg, &icon_02d);
                    lv_obj_align_to(afternoonIconImg, afternoonIcon, LV_ALIGN_CENTER, 0, 0);
                }
                lv_obj_clear_flag(afternoonIconImg, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
    
    Serial.printf("[INFO] Updated afternoon forecast: %.1f°C, Rain: %.0f%%\n", 
                 temp, pop * 100);
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

void UIManager::createSettingsScreen() {
    if (settingsScreen) {
        lv_obj_del(settingsScreen);
    }
    
    settingsScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(settingsScreen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(settingsScreen, lv_color_black(), LV_PART_MAIN);
    
    // Create a transparent area at the top for back/home button functionality
    settingsBackArea = lv_obj_create(settingsScreen);
    lv_obj_set_size(settingsBackArea, 800, 50);
    lv_obj_align(settingsBackArea, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(settingsBackArea, LV_OPA_0, LV_PART_MAIN); // Make fully transparent
    lv_obj_set_style_border_width(settingsBackArea, 0, LV_PART_MAIN);
    
    // Add click event to the transparent area to go back to home screen
    lv_obj_add_event_cb(settingsBackArea, back_area_clicked_cb, LV_EVENT_CLICKED, this);
    
    // Title
    lv_obj_t* title = lv_label_create(settingsScreen);
    lv_obj_add_style(title, &infoStyle, 0);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_clear_flag(title, LV_OBJ_FLAG_CLICKABLE); // Make title non-clickable
    
    // Container for buttons
    lv_obj_t* btnContainer = lv_obj_create(settingsScreen);
    lv_obj_remove_style_all(btnContainer);
    lv_obj_set_size(btnContainer, lv_pct(70), 300);
    lv_obj_align(btnContainer, LV_ALIGN_CENTER, 0, 20);
    lv_obj_clear_flag(btnContainer, LV_OBJ_FLAG_CLICKABLE); // Make container non-clickable
    
    // Alarm button
    lv_obj_t* btnAlarm = lv_btn_create(btnContainer);
    lv_obj_add_style(btnAlarm, &buttonStyle, 0);
    lv_obj_add_style(btnAlarm, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnAlarm, lv_pct(100), 60);
    lv_obj_align(btnAlarm, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t* alarmLabel = lv_label_create(btnAlarm);
    lv_label_set_text(alarmLabel, "Alarm Settings");
    lv_obj_center(alarmLabel);
    
    // Use a static member function as callback instead of a capturing lambda
    lv_obj_add_event_cb(btnAlarm, alarm_btn_clicked_cb, LV_EVENT_CLICKED, this);
    
    // Radio button
    lv_obj_t* btnRadio = lv_btn_create(btnContainer);
    lv_obj_add_style(btnRadio, &buttonStyle, 0);
    lv_obj_add_style(btnRadio, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnRadio, lv_pct(100), 60);
    lv_obj_align(btnRadio, LV_ALIGN_TOP_MID, 0, 80);
    
    lv_obj_t* radioLabel = lv_label_create(btnRadio);
    lv_label_set_text(radioLabel, "Radio");
    lv_obj_center(radioLabel);
    
    // Use a static member function as callback instead of a capturing lambda
    lv_obj_add_event_cb(btnRadio, radio_btn_clicked_cb, LV_EVENT_CLICKED, this);
    
    // Weather button
    lv_obj_t* btnWeather = lv_btn_create(btnContainer);
    lv_obj_add_style(btnWeather, &buttonStyle, 0);
    lv_obj_add_style(btnWeather, &buttonPressedStyle, LV_STATE_PRESSED);
    lv_obj_set_size(btnWeather, lv_pct(100), 60);
    lv_obj_align(btnWeather, LV_ALIGN_TOP_MID, 0, 160);
    
    lv_obj_t* weatherLabel = lv_label_create(btnWeather);
    lv_label_set_text(weatherLabel, "Weather");
    lv_obj_center(weatherLabel);
    
    // Use a static member function as callback instead of a capturing lambda
    lv_obj_add_event_cb(btnWeather, weather_btn_clicked_cb, LV_EVENT_CLICKED, this);
}

void UIManager::back_area_clicked_cb(lv_event_t* e) {
    Serial.println("Back area clicked - returning to home screen");
    UIManager* self = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (self) {
        self->showHomeScreen();
    }
}

void UIManager::alarm_btn_clicked_cb(lv_event_t* e) {
    Serial.println("Alarm button clicked");
    UIManager* self = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (self) {
        self->showAlarmSettingsScreen();
    }
}

void UIManager::radio_btn_clicked_cb(lv_event_t* e) {
    Serial.println("Radio button clicked");
    UIManager* self = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (self) {
        self->showRadioScreen();
    }
}

void UIManager::weather_btn_clicked_cb(lv_event_t* e) {
    Serial.println("Weather button clicked");
    UIManager* self = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (self) {
        // Weather screen isn't implemented yet, so we just log it
        Serial.println("Weather screen not implemented yet");
    }
}

void UIManager::back_btn_clicked_cb(lv_event_t* e) {
    Serial.println("Back button clicked");
    UIManager* self = static_cast<UIManager*>(lv_event_get_user_data(e));
    if (self) {
        self->showHomeScreen();
    }
}

void UIManager::showScreen(lv_obj_t* screen) {
    // Basic version with minimal operations
    if (!screen) {
        Serial.println("ERROR: Attempted to show null screen");
        return;
    }
    
    // Make sure LVGL is initialized
    if (!lv_is_initialized()) {
        Serial.println("ERROR: LVGL not initialized, can't show screen");
        return;
    }
    
    // Update current screen tracking
    currentScreen = screen;
    
    // Simple screen load with no animations
    lv_scr_load(screen);
    
    Serial.println("Screen loaded");
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
    Serial.println("showSettingsScreen() called");
    
    // Make sure settingsScreen exists
    if (!settingsScreen) {
        Serial.println("Creating settings screen since it doesn't exist");
        createSettingsScreen();
        if (!settingsScreen) {
            Serial.println("ERROR: Failed to create settings screen!");
            return;
        }
    } else {
        Serial.println("Settings screen already exists");
        
        // If the screen already exists, reset the timer
        if (settingsScreenTimer) {
            lv_timer_reset(settingsScreenTimer);
        } else {
            // Create a new timer if needed
            settingsScreenTimer = lv_timer_create([](lv_timer_t* timer) {
                UIManager* ui = static_cast<UIManager*>(timer->user_data);
                if (ui) {
                    ui->showHomeScreen();
                }
                // Delete the timer after it fires
                lv_timer_del(timer);
            }, 10000, this);  // 10000ms = 10 seconds
        }
    }
    
    // Load the settings screen with simple load (no animation)
    lv_scr_load(settingsScreen);
    
    // Update current screen tracking
    currentScreen = settingsScreen;
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
    // Initialize styles for main UI elements with custom fonts for German umlauts
    lv_style_init(&infoStyle);
    lv_style_set_text_font(&infoStyle, &lv_font_montserrat_16x);  // Custom font with umlauts
    lv_style_set_text_color(&infoStyle, lv_color_hex(0xCCCCCC));
    
    lv_style_init(&statusStyle);
    lv_style_set_text_font(&statusStyle, &lv_font_montserrat_14);
    lv_style_set_text_color(&statusStyle, lv_color_hex(0xAAAAAA));
    
    lv_style_init(&timeStyle);
    lv_style_set_text_font(&timeStyle, &lv_font_montserrat_48);
    lv_style_set_text_color(&timeStyle, lv_color_white());
    
    lv_style_init(&dateStyle);
    lv_style_set_text_font(&dateStyle, &lv_font_montserrat_16x);  // Custom font with umlauts
    lv_style_set_text_color(&dateStyle, lv_color_hex(0xCCCCCC));
    
    // Weather panel styles
    lv_style_init(&panelStyle);
    lv_style_set_bg_color(&panelStyle, lv_color_hex(0x1D232B)); // Darker blue
    lv_style_set_radius(&panelStyle, 10);
    lv_style_set_pad_all(&panelStyle, 10);
    lv_style_set_border_width(&panelStyle, 0);
    
    lv_style_init(&titleStyle);
    lv_style_set_text_font(&titleStyle, &lv_font_montserrat_20x);  // Custom font with umlauts
    lv_style_set_text_color(&titleStyle, lv_color_white());
    
    lv_style_init(&valueStyle);
    lv_style_set_text_font(&valueStyle, &lv_font_montserrat_24);
    lv_style_set_text_color(&valueStyle, lv_color_white());
    
    lv_style_init(&iconStyle);
    lv_style_set_text_font(&iconStyle, &lv_font_montserrat_32);
    lv_style_set_text_color(&iconStyle, lv_color_white());
    
    lv_style_init(&weatherIconStyle);
    lv_style_set_text_font(&weatherIconStyle, &lv_font_montserrat_40x);  // Custom font with weather symbols
    lv_style_set_text_color(&weatherIconStyle, lv_color_white());
    
    // Button styles - enhanced for better visibility and touch target
    lv_style_init(&buttonStyle);
    lv_style_set_bg_color(&buttonStyle, lv_color_hex(0x2196F3)); // Blue
    lv_style_set_text_color(&buttonStyle, lv_color_white());
    lv_style_set_radius(&buttonStyle, 10);
    lv_style_set_border_width(&buttonStyle, 3); // Thicker border
    lv_style_set_border_color(&buttonStyle, lv_color_hex(0x1976D2)); // Darker blue
    lv_style_set_shadow_width(&buttonStyle, 8); // Larger shadow
    lv_style_set_shadow_ofs_y(&buttonStyle, 5);
    lv_style_set_pad_all(&buttonStyle, 10); // More padding for easier touch target
    lv_style_set_text_font(&buttonStyle, &lv_font_montserrat_16x);  // Custom font with umlauts
    
    lv_style_init(&buttonPressedStyle);
    lv_style_set_bg_color(&buttonPressedStyle, lv_color_hex(0x1976D2)); // Darker blue
    lv_style_set_text_color(&buttonPressedStyle, lv_color_white());
    lv_style_set_shadow_width(&buttonPressedStyle, 0);
    lv_style_set_bg_opa(&buttonPressedStyle, LV_OPA_80);
    
    // Day button styles for alarm settings
    lv_style_init(&dayButtonStyle);
    lv_style_set_bg_color(&dayButtonStyle, lv_color_hex(0x2D4358)); // Blue gray
    lv_style_set_text_color(&dayButtonStyle, lv_color_white());
    lv_style_set_border_width(&dayButtonStyle, 0);
    lv_style_set_outline_width(&dayButtonStyle, 0);
    lv_style_set_radius(&dayButtonStyle, 5);
    lv_style_set_height(&dayButtonStyle, 20);
    lv_style_set_width(&dayButtonStyle, 30);
    
    lv_style_init(&dayButtonActiveStyle);
    lv_style_set_bg_color(&dayButtonActiveStyle, lv_color_hex(0x3E5A7A)); // Lighter blue
    
    // Set the default screen color
    static lv_style_t screen_style;
    lv_style_init(&screen_style);
    lv_style_set_pad_all(&screen_style, 0);
    lv_style_set_bg_color(&screen_style, lv_color_black());
    lv_style_set_text_color(&screen_style, lv_color_white());
    lv_style_set_border_width(&screen_style, 0);
    
    lv_obj_add_style(lv_scr_act(), &screen_style, 0);
    
    // Set default theme with custom font
    lv_theme_t* theme = lv_theme_default_init(
        lv_disp_get_default(),
        lv_palette_main(LV_PALETTE_BLUE),
        lv_palette_main(LV_PALETTE_RED),
        darkTheme,  // Use dark theme
        &lv_font_montserrat_16x  // Custom font with umlauts as default font
    );
    lv_disp_set_theme(lv_disp_get_default(), theme);
}

void UIManager::createHomeScreen() {
    Serial.println("Creating home screen");
    
    // Delete old screen if it exists to start fresh
    if (homeScreen) {
        lv_obj_del(homeScreen);
        homeScreen = NULL;
    }
    
    // Create a fresh home screen
    homeScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(homeScreen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_size(homeScreen, 800, 480);
    // Add multiple event types to ensure we catch all touch interactions
    // First try PRESSED event (triggers as soon as finger touches screen)
    lv_obj_add_event_cb(homeScreen, [](lv_event_t* e) {
        Serial.println("Home screen PRESSED event detected");
        UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
        if (ui) ui->showSettingsScreen();
    }, LV_EVENT_PRESSED, this);
    
    // Also add CLICKED event as backup
    lv_obj_add_event_cb(homeScreen, [](lv_event_t* e) {
        Serial.println("Home screen CLICKED event detected");
        UIManager* ui = static_cast<UIManager*>(lv_event_get_user_data(e));
        if (ui) ui->showSettingsScreen();
    }, LV_EVENT_CLICKED, this);
    
    // Make sure home screen can receive touch events
    lv_obj_add_flag(homeScreen, LV_OBJ_FLAG_CLICKABLE);
    
    // Create status bar at the top
    lv_obj_t* statusBar = lv_obj_create(homeScreen);
    lv_obj_set_size(statusBar, 800, 35);
    lv_obj_align(statusBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(statusBar, lv_color_hex(0x111111), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(statusBar, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(statusBar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(statusBar, 0, LV_PART_MAIN);
    // Make this panel transparent to clicks so they reach the home screen
    lv_obj_clear_flag(statusBar, LV_OBJ_FLAG_CLICKABLE);
    
    // WiFi status in the left section of status bar
    wifiLabel = lv_label_create(statusBar);
    lv_obj_add_style(wifiLabel, &statusStyle, 0);
    lv_label_set_text(wifiLabel, "WiFi: --");
    lv_obj_align(wifiLabel, LV_ALIGN_LEFT_MID, 10, 0);
    
    // IP address in the middle section
    ipLabel = lv_label_create(statusBar);
    lv_obj_add_style(ipLabel, &statusStyle, 0);
    lv_label_set_text(ipLabel, "IP: --");
    lv_obj_align(ipLabel, LV_ALIGN_CENTER, 0, 0);
    
    // WiFi quality in the right section
    wifiQualityLabel = lv_label_create(statusBar);
    lv_obj_add_style(wifiQualityLabel, &statusStyle, 0);
    lv_label_set_text(wifiQualityLabel, LV_SYMBOL_WIFI " --");
    lv_obj_align(wifiQualityLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    
    // Create a large panel for the time and date - spans from below status bar to sensor box, horizontally to weather box
    lv_obj_t* timePanel = lv_obj_create(homeScreen);
    // Calculate size: Width is screen width (800) minus weather panel width (200) minus margin (5)
    // Height is from status bar (35px) to sensor box (position is 480-70-20 = 390px) minus margins
    lv_obj_set_size(timePanel, 595, 345); // 800-200-5, 390-35-10
    lv_obj_align(timePanel, LV_ALIGN_TOP_LEFT, 0, 35); // Position right below status bar
    // Make this panel transparent to clicks so they reach the home screen
    lv_obj_clear_flag(timePanel, LV_OBJ_FLAG_CLICKABLE);
    
    // No dedicated settings button for now to ensure stability
    lv_obj_set_style_bg_color(timePanel, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(timePanel, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(timePanel, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(timePanel, lv_color_hex(0x0000FF), LV_PART_MAIN);
    lv_obj_set_style_radius(timePanel, 10, LV_PART_MAIN);
    
    // Create time label with large font
    timeLabel = lv_label_create(timePanel);
    lv_obj_add_style(timeLabel, &timeStyle, 0);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, 0); // Use largest font
    lv_label_set_text(timeLabel, "--:--:--");
    lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -20);
    
    // Date label (format will be "Weekday dd.mm.yyyy" with weekday in German)
    dateLabel = lv_label_create(timePanel);
    lv_obj_add_style(dateLabel, &dateStyle, 0);
    lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_24, 0);
    lv_label_set_text(dateLabel, "--.--.----");
    lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 30);
    
    // Next alarm label below the date
    nextAlarmLabel = lv_label_create(timePanel);
    lv_obj_add_style(nextAlarmLabel, &infoStyle, 0);
    lv_label_set_text(nextAlarmLabel, "No Alarms");
    lv_obj_align(nextAlarmLabel, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    // Create sensor panel with better spacing - moved to bottom-left, 70% width
    lv_obj_t* sensorPanel = lv_obj_create(homeScreen);
    lv_obj_set_size(sensorPanel, 560, 70);  // 70% of 800px width = 560px
    lv_obj_align(sensorPanel, LV_ALIGN_BOTTOM_LEFT, 5, -20);  // Position at bottom-left with margin
    // Make this panel transparent to clicks so they reach the home screen
    lv_obj_clear_flag(sensorPanel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(sensorPanel, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sensorPanel, LV_OPA_70, LV_PART_MAIN);  // More opaque for better readability
    lv_obj_set_style_border_width(sensorPanel, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(sensorPanel, lv_color_hex(0x444444), LV_PART_MAIN);  // Darker border
    lv_obj_set_style_radius(sensorPanel, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(sensorPanel, 10, LV_PART_MAIN);  // Add padding inside panel
    
    // Create a grid layout for the sensor panel
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    
    lv_obj_set_grid_dsc_array(sensorPanel, col_dsc, row_dsc);
    lv_obj_set_style_grid_row_align(sensorPanel, LV_GRID_ALIGN_SPACE_BETWEEN, 0);
    lv_obj_set_style_grid_column_align(sensorPanel, LV_GRID_ALIGN_SPACE_EVENLY, 0);
    
    // Create weather panel - 25% width, full height from status bar to bottom
    weatherPanel = lv_obj_create(homeScreen);
    lv_obj_set_size(weatherPanel, 200, 445);  // 25% of 800px = 200px, height from status bar (35px) to bottom (480-5px margin)
    lv_obj_align(weatherPanel, LV_ALIGN_TOP_RIGHT, -5, 35);  // Position at right side from status bar to bottom
    // Make this panel transparent to clicks so they reach the home screen
    lv_obj_clear_flag(weatherPanel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(weatherPanel, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(weatherPanel, LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_border_width(weatherPanel, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(weatherPanel, lv_color_hex(0x0088FF), LV_PART_MAIN);  // Blue border
    lv_obj_set_style_radius(weatherPanel, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(weatherPanel, 5, LV_PART_MAIN);  // Reduced padding for more space
    
    // Title for the current weather
    currentWeatherTitle = lv_label_create(weatherPanel);
    lv_obj_add_style(currentWeatherTitle, &infoStyle, 0);
    lv_label_set_text(currentWeatherTitle, "Aktuelles Wetter");
    lv_obj_align(currentWeatherTitle, LV_ALIGN_TOP_MID, 0, 2);
    
    // Weather icon - centered for narrow panel
    // Create a container for the weather icon (will contain both label and image)
    lv_obj_t* weatherIconContainer = lv_obj_create(weatherPanel);
    lv_obj_remove_style_all(weatherIconContainer);  // Remove default styles
    lv_obj_set_size(weatherIconContainer, 50, 50);  // Smaller container
    lv_obj_align(weatherIconContainer, LV_ALIGN_TOP_MID, 0, 25);  // Moved up
    
    // Create the label (fallback)
    weatherIcon = lv_label_create(weatherIconContainer);
    lv_obj_set_style_text_font(weatherIcon, &lv_font_montserrat_40, 0);  // Slightly smaller font
    lv_label_set_text(weatherIcon, "");  // Empty text, we'll use the image instead
    lv_obj_center(weatherIcon);
    
    // The image will be created in updateCurrentWeather when we have the actual icon data
    weatherIconImg = NULL;  // Will be created when needed
    
    // Current temperature
    currentTempLabel = lv_label_create(weatherPanel);
    lv_obj_set_style_text_font(currentTempLabel, &lv_font_montserrat_28, 0);  // Slightly smaller font
    lv_label_set_text(currentTempLabel, "--°C");
    lv_obj_align(currentTempLabel, LV_ALIGN_TOP_MID, 0, 75);  // Moved up
    
    // Feels like temperature
    feelsLikeLabel = lv_label_create(weatherPanel);
    lv_obj_add_style(feelsLikeLabel, &infoStyle, 0);
    lv_label_set_text(feelsLikeLabel, "Gefühlt: --°C");
    lv_obj_align(feelsLikeLabel, LV_ALIGN_TOP_MID, 0, 105);  // Moved up
    
    // Weather description
    weatherDescLabel = lv_label_create(weatherPanel);
    lv_obj_add_style(weatherDescLabel, &infoStyle, 0);
    lv_label_set_text(weatherDescLabel, "Keine Daten");
    lv_obj_align(weatherDescLabel, LV_ALIGN_TOP_MID, 0, 130);  // Moved up
    
    // Create forecast panel within the weather panel
    forecastPanel = lv_obj_create(weatherPanel);
    lv_obj_set_size(forecastPanel, 190, 250); // Reduced height to fit better below current weather
    lv_obj_align(forecastPanel, LV_ALIGN_BOTTOM_MID, 0, -5);  // Closer to bottom
    lv_obj_set_style_bg_color(forecastPanel, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(forecastPanel, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_border_width(forecastPanel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(forecastPanel, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_radius(forecastPanel, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_all(forecastPanel, 4, LV_PART_MAIN);  // Slightly less padding
    
    // Setup a more compact layout for the forecast panel
    
    // Add divider line between morning and afternoon forecasts
    lv_obj_t* dividerLine = lv_obj_create(forecastPanel);
    lv_obj_set_size(dividerLine, 170, 2);  // Nearly full width, 2px height
    lv_obj_align(dividerLine, LV_ALIGN_CENTER, 0, -5);  // Moved slightly up to allow more space below
    lv_obj_set_style_bg_color(dividerLine, lv_color_hex(0x666666), LV_PART_MAIN);
    lv_obj_set_style_border_width(dividerLine, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(dividerLine, 0, LV_PART_MAIN);
    
    // Morning forecast - stacked vertically in top half
    morningTitle = lv_label_create(forecastPanel);
    lv_obj_add_style(morningTitle, &infoStyle, 0);
    lv_label_set_text(morningTitle, "Vormittag");
    lv_obj_align(morningTitle, LV_ALIGN_TOP_MID, 0, 2);  // Reduced top margin
    
    // Create a container for the morning forecast icon
    lv_obj_t* morningIconContainer = lv_obj_create(forecastPanel);
    lv_obj_remove_style_all(morningIconContainer);
    lv_obj_set_size(morningIconContainer, 40, 40);
    lv_obj_align(morningIconContainer, LV_ALIGN_TOP_MID, 0, 22);  // Reduced spacing
    
    // Create the label (fallback)
    morningIcon = lv_label_create(morningIconContainer);
    lv_obj_set_style_text_font(morningIcon, &lv_font_montserrat_20, 0);
    lv_label_set_text(morningIcon, "");  // Empty text, we'll use the image instead
    lv_obj_center(morningIcon);
    
    // The image will be created in updateMorningForecast when we have the actual icon data
    morningIconImg = NULL;  // Will be created when needed
    
    morningTempLabel = lv_label_create(forecastPanel);
    lv_obj_add_style(morningTempLabel, &infoStyle, 0);
    lv_label_set_text(morningTempLabel, "--°C");
    lv_obj_align(morningTempLabel, LV_ALIGN_TOP_MID, 0, 65);  // Moved up
    
    morningRainLabel = lv_label_create(forecastPanel);
    lv_obj_add_style(morningRainLabel, &infoStyle, 0);
    lv_label_set_text(morningRainLabel, "Regen: --%");
    lv_obj_align(morningRainLabel, LV_ALIGN_TOP_MID, 0, 85);  // Moved up
    
    // Afternoon title - in bottom half
    afternoonTitle = lv_label_create(forecastPanel);
    lv_obj_add_style(afternoonTitle, &infoStyle, 0);
    lv_label_set_text(afternoonTitle, "Nachmittag");
    lv_obj_align(afternoonTitle, LV_ALIGN_TOP_MID, 0, 130);  // Moved up
    
    // Create a container for the afternoon forecast icon
    lv_obj_t* afternoonIconContainer = lv_obj_create(forecastPanel);
    lv_obj_remove_style_all(afternoonIconContainer);
    lv_obj_set_size(afternoonIconContainer, 40, 40);
    lv_obj_align(afternoonIconContainer, LV_ALIGN_TOP_MID, 0, 150);  // Moved up
    
    // Create the label (fallback)
    afternoonIcon = lv_label_create(afternoonIconContainer);
    lv_obj_set_style_text_font(afternoonIcon, &lv_font_montserrat_20, 0);
    lv_label_set_text(afternoonIcon, "");  // Empty text, we'll use the image instead
    lv_obj_center(afternoonIcon);
    
    // The image will be created in updateAfternoonForecast when we have the actual icon data
    afternoonIconImg = NULL;  // Will be created when needed
    
    afternoonTempLabel = lv_label_create(forecastPanel);
    lv_obj_add_style(afternoonTempLabel, &infoStyle, 0);
    lv_label_set_text(afternoonTempLabel, "--°C");
    lv_obj_align(afternoonTempLabel, LV_ALIGN_TOP_MID, 0, 193);  // Moved up
    
    afternoonRainLabel = lv_label_create(forecastPanel);
    lv_obj_add_style(afternoonRainLabel, &infoStyle, 0);
    lv_label_set_text(afternoonRainLabel, "Regen: --%");
    lv_obj_align(afternoonRainLabel, LV_ALIGN_TOP_MID, 0, 213);  // Moved up
    
    lv_obj_set_grid_dsc_array(sensorPanel, col_dsc, row_dsc);
    lv_obj_set_style_pad_all(sensorPanel, 5, LV_PART_MAIN);  // Keep reduced padding
    lv_obj_set_style_pad_column(sensorPanel, 8, LV_PART_MAIN); // Adjust column spacing
    lv_obj_set_style_pad_row(sensorPanel, 2, LV_PART_MAIN); // Minimal row spacing
    
    // Style for sensor titles
    static lv_style_t title_style;
    lv_style_init(&title_style);
    lv_style_set_text_font(&title_style, &lv_font_montserrat_12);
    lv_style_set_text_color(&title_style, lv_color_white());
    
    // Style for sensor values
    static lv_style_t value_style;
    lv_style_init(&value_style);
    lv_style_set_text_font(&value_style, &lv_font_montserrat_20);
    lv_style_set_text_color(&value_style, lv_color_hex(0x00FF00));
    
    // Temperature
    lv_obj_t* tempTitle = lv_label_create(sensorPanel);
    lv_obj_add_style(tempTitle, &title_style, 0);
    lv_label_set_text(tempTitle, "TEMPERATURE");
    lv_obj_set_style_text_align(tempTitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(tempTitle, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_START, 0, 1);
    
    tempLabel = lv_label_create(sensorPanel);
    lv_obj_add_style(tempLabel, &value_style, 0);
    lv_label_set_text(tempLabel, "--°C");
    lv_obj_set_style_text_align(tempLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(tempLabel, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    
    // Humidity
    lv_obj_t* humTitle = lv_label_create(sensorPanel);
    lv_obj_add_style(humTitle, &title_style, 0);
    lv_label_set_text(humTitle, "HUMIDITY");
    lv_obj_set_style_text_align(humTitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(humTitle, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 0, 1);
    
    humidityLabel = lv_label_create(sensorPanel);
    lv_obj_add_style(humidityLabel, &value_style, 0);
    lv_label_set_text(humidityLabel, "--%");
    lv_obj_set_style_text_align(humidityLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(humidityLabel, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    
    // CO2
    lv_obj_t* co2Title = lv_label_create(sensorPanel);
    lv_obj_add_style(co2Title, &title_style, 0);
    lv_label_set_text(co2Title, "CO2");
    lv_obj_set_style_text_align(co2Title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(co2Title, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_START, 0, 1);
    
    eco2Label = lv_label_create(sensorPanel);
    lv_obj_add_style(eco2Label, &value_style, 0);
    lv_label_set_text(eco2Label, "---");
    lv_obj_set_style_text_align(eco2Label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(eco2Label, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    
    // TVOC
    lv_obj_t* tvocTitle = lv_label_create(sensorPanel);
    lv_obj_add_style(tvocTitle, &title_style, 0);
    lv_label_set_text(tvocTitle, "TVOC");
    lv_obj_set_style_text_align(tvocTitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(tvocTitle, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_START, 0, 1);
    
    tvocLabel = lv_label_create(sensorPanel);
    lv_obj_add_style(tvocLabel, &value_style, 0);
    lv_label_set_text(tvocLabel, "---");
    lv_obj_set_style_text_align(tvocLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(tvocLabel, LV_GRID_ALIGN_STRETCH, 3, 1, LV_GRID_ALIGN_CENTER, 1, 1);
    
    Serial.println("Home screen created successfully");
    
    // No need to force a full screen update here, LVGL will update when needed
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
}

// Navigation button callback implementation
void UIManager::nav_btn_clicked_cb(lv_event_t* e) {
    Serial.println("****** NAVIGATION BUTTON CLICKED ******");
    
    // Try first to interpret as UserData struct
    UserData* userData = static_cast<UserData*>(lv_event_get_user_data(e));
    
    // Variables to hold our navigation data
    UIManager* ui = nullptr;
    int btnType = 0;
    
    if (userData && userData->ui) {
        // Use UserData struct if available
        ui = userData->ui;
        btnType = userData->value;
        Serial.printf("Using UserData struct: button type = %d\n", btnType);
    } else {
        // Fall back to old int* approach
        int* btnTypePtr = static_cast<int*>(lv_event_get_user_data(e));
        if (!btnTypePtr) {
            Serial.println("Error: Invalid navigation button data");
            return;
        }
        
        // Use singleton instance as fallback
        ui = &UIManager::getInstance();
        btnType = *btnTypePtr;
        Serial.printf("Using int* pointer: button type = %d\n", btnType);
    }
    
    if (!ui) {
        Serial.println("Error: No valid UIManager instance found");
        return;
    }
    
    // Navigate based on button type
    switch (btnType) {
        case 1: // Alarm
            Serial.println("Alarm button pressed - showing alarm settings");
            ui->showAlarmSettingsScreen();
            break;
        case 2: // Radio
            Serial.println("Radio button pressed - showing radio screen");
            ui->showRadioScreen();
            break;
        case 3: // Settings button
            Serial.println("Settings button pressed - showing settings screen");
            ui->showSettingsScreen();
            break;
        default:
            Serial.printf("Unknown button type: %d\n", btnType);
            break;
    }
    
    // Process LVGL timers to handle the UI update
    lv_timer_handler();
}
