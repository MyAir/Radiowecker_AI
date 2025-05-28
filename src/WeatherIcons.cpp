#include "WeatherIcons.h"
#include <Arduino.h>  // For Serial support
#include <string.h>
#include <cstdio>
#include <lvgl.h>

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

// Function to create a style for the weather icons with proper transparency
static void apply_weather_icon_style(lv_obj_t *img) {
    if (!img) return;
    
    // Remove any existing styles
    lv_obj_remove_style_all(img);
    
    // Set basic image properties
    lv_obj_set_style_opa(img, LV_OPA_COVER, 0);
    lv_obj_set_style_img_opa(img, LV_OPA_COVER, 0);
    
    // Ensure the background is transparent
    lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(img, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_opa(img, LV_OPA_TRANSP, 0);
    
    // Remove padding
    lv_obj_set_style_pad_all(img, 0, 0);
    // Margins are handled by the parent container
    
    // Disable image recoloring
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_TRANSP, 0);
    
    // Set blend mode to normal and ensure proper color handling
    lv_obj_set_style_blend_mode(img, LV_BLEND_MODE_NORMAL, 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor(img, lv_color_white(), 0);
    lv_img_set_pivot(img, 0, 0);
    lv_obj_set_style_img_opa(img, LV_OPA_COVER, 0);
    
    // Reset any transformations
    lv_img_set_angle(img, 0);
    lv_img_set_zoom(img, 256);  // 256 = 100% zoom
    
    // Clear only the essential flags
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLL_CHAIN);
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

// Helper function to create and display a weather icon
lv_obj_t* create_weather_icon(lv_obj_t* parent, const char* iconCode) {
    if (!parent || !iconCode) {
        Serial.println("[ERROR] Invalid parameters for create_weather_icon");
        return nullptr;
    }
    
    Serial.printf("[DEBUG] Creating weather icon for code: %s\n", iconCode);
    
    // Get the icon descriptor
    const lv_img_dsc_t* icon_dsc = get_weather_icon(iconCode);
    if (!icon_dsc) {
        Serial.println("[ERROR] Failed to get weather icon descriptor");
        return nullptr;
    }
    
    // Log icon details for debugging
    Serial.printf("[DEBUG] Icon details - w:%d, h:%d, size:%d, cf:0x%02X\n", 
                 icon_dsc->header.w, icon_dsc->header.h, 
                 icon_dsc->data_size, icon_dsc->header.cf);
    
    // Create the image object
    lv_obj_t* img = lv_img_create(parent);
    if (!img) {
        Serial.println("[ERROR] Failed to create image object");
        return nullptr;
    }
    
    // Remove any default styles that might interfere
    lv_obj_remove_style_all(img);
    
    // Set the image source
    lv_img_set_src(img, icon_dsc);
    
    // Set the image size to match the icon size
    lv_obj_set_size(img, icon_dsc->header.w, icon_dsc->header.h);
    
    // Apply the style with proper transparency handling
    apply_weather_icon_style(img);
    
    // Set image properties for transparency and correct colors
    lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(img, LV_OPA_TRANSP, 0);
    lv_obj_set_style_outline_opa(img, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(img, 0, 0);
    lv_obj_set_style_radius(img, 0, 0);
    lv_obj_set_style_clip_corner(img, false, 0);
    lv_obj_set_style_img_opa(img, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_TRANSP, 0);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, 0);
    lv_obj_set_style_img_recolor(img, lv_color_white(), 0);
    lv_obj_set_style_blend_mode(img, LV_BLEND_MODE_NORMAL, 0);
    
    // Center the image in its parent
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    
    // Force a refresh of the display
    lv_obj_invalidate(img);
    
    Serial.println("[DEBUG] Weather icon created and configured successfully");
    
    return img;
}
