#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include <TAMC_GT911.h>
#include <Wire.h>
#include "DisplayConfig.h"

// Forward declarations
class TAMC_GT911;
class Arduino_ESP32RGBPanel;
class Arduino_RGB_Display;

class DisplayManager {
private:
    static DisplayManager* instance;
    
    // Private constructor
    DisplayManager();
    
    // Prevent copying and assignment
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;
    
public:
    static DisplayManager& getInstance() {
        if (!instance) {
            instance = new DisplayManager();
        }
        return *instance;
    }

    /**
     * @brief Perform a hardware reset of the display and touch components
     * This helps resolve initialization issues by resetting all display-related pins
     */
    void performHardwareReset();
    
    /**
     * @brief Initialize the display manager
     * @return true if initialization was successful, false otherwise
     */
    bool begin();
    
    /**
     * @brief Set the display brightness
     * @param brightness Brightness level (0-255)
     */
    void setBrightness(uint8_t brightness);
    
    // Get the current brightness level (0-255)
    uint8_t getCurrentBrightness() const { return currentBrightness; }
    
    /**
     * @brief Update the display (call this in the main loop)
     */
    void update();
    
    // LVGL display and touch callbacks
    static void lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void lvgl_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
    
    // Getters
    Arduino_GFX* getGfx() { return gfx; }
    TAMC_GT911* getTouch() { return touch; }
    
    /**
     * @brief Check if touch controller is initialized and working
     * @return true if touch controller is working, false otherwise
     */
    bool isTouchWorking() const { return touch != nullptr && touch_initialized; }

private:
    ~DisplayManager() = default;
    bool initDisplay();
    bool initTouch();
    bool initLVGL();

    // Display and touch instances
    Arduino_ESP32RGBPanel *bus = nullptr;
    Arduino_GFX *gfx = nullptr; // Using the base class to allow for different display implementations
    TAMC_GT911 *touch = nullptr;
    bool touch_initialized = false;

    // LVGL display buffer
    static constexpr uint16_t screenWidth = SCREEN_WIDTH;
    static constexpr uint16_t screenHeight = SCREEN_HEIGHT;
    static constexpr uint16_t bufferSize = screenWidth * 40; // Adjust based on available memory
    lv_color_t *lv_display_buf1 = nullptr;
    lv_color_t *lv_display_buf2 = nullptr;
    lv_disp_draw_buf_t draw_buf;
    lv_disp_drv_t disp_drv;
    lv_indev_drv_t indev_drv;
    lv_disp_t *disp = nullptr;
    lv_indev_t *indev_touch = nullptr;

    // Touch state
    static bool touch_has_signal;
    static int16_t touch_last_x;
    static int16_t touch_last_y;
    
    // Touch controller instance
    static TAMC_GT911* touch_controller;
    
    // Current brightness (0-100)
    uint8_t currentBrightness;
    
    // Touch controller methods
    bool readTouch(int16_t& x, int16_t& y);
    bool isTouched();
};

// Initialize static member
inline DisplayManager* DisplayManager::instance = nullptr;
