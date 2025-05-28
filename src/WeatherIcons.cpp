#include "WeatherIcons.h"
#include <string.h>
#include <cstdio>

// Forward declarations for the icon data
extern "C" {
    extern const lv_img_dsc_t icon_01d;
    extern const lv_img_dsc_t icon_01n;
    extern const lv_img_dsc_t icon_02d;
    extern const lv_img_dsc_t icon_02n;
    extern const lv_img_dsc_t icon_03d;
    extern const lv_img_dsc_t icon_03n;
    extern const lv_img_dsc_t icon_04d;
    extern const lv_img_dsc_t icon_04n;
    extern const lv_img_dsc_t icon_09d;
    extern const lv_img_dsc_t icon_09n;
    extern const lv_img_dsc_t icon_10d;
    extern const lv_img_dsc_t icon_10n;
    extern const lv_img_dsc_t icon_11d;
    extern const lv_img_dsc_t icon_11n;
    extern const lv_img_dsc_t icon_13d;
    extern const lv_img_dsc_t icon_13n;
    extern const lv_img_dsc_t icon_50d;
    extern const lv_img_dsc_t icon_50n;
}

// Get the appropriate weather icon based on the OpenWeatherMap icon code
const lv_img_dsc_t* get_weather_icon(const char* iconCode) {
    static char defaultCode[8] = "01d";
    
    // If no icon code provided, return a default icon
    if (!iconCode || strlen(iconCode) < 3) {
        iconCode = defaultCode;
    }
    
    // Map OpenWeatherMap icon codes to our icon variables
    if (strcmp(iconCode, "01d") == 0) return &icon_01d;
    if (strcmp(iconCode, "01n") == 0) return &icon_01n;
    if (strcmp(iconCode, "02d") == 0) return &icon_02d;
    if (strcmp(iconCode, "02n") == 0) return &icon_02n;
    if (strcmp(iconCode, "03d") == 0 || strcmp(iconCode, "03n") == 0) return &icon_03d;
    if (strcmp(iconCode, "04d") == 0 || strcmp(iconCode, "04n") == 0) return &icon_04d;
    if (strcmp(iconCode, "09d") == 0 || strcmp(iconCode, "09n") == 0) return &icon_09d;
    if (strcmp(iconCode, "10d") == 0) return &icon_10d;
    if (strcmp(iconCode, "10n") == 0) return &icon_10n;
    if (strcmp(iconCode, "11d") == 0 || strcmp(iconCode, "11n") == 0) return &icon_11d;
    if (strcmp(iconCode, "13d") == 0 || strcmp(iconCode, "13n") == 0) return &icon_13d;
    if (strcmp(iconCode, "50d") == 0 || strcmp(iconCode, "50n") == 0) return &icon_50d;
    
    // Default to day clear icon if no match found
    return &icon_01d;
}
