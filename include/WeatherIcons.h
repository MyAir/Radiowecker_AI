#ifndef WEATHER_ICONS_H
#define WEATHER_ICONS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for the weather icon data
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

// Function to get the appropriate weather icon
const lv_img_dsc_t* get_weather_icon(const char* iconCode);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WEATHER_ICONS_H
