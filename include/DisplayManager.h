#pragma once

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>
#include "DisplayConfig.h"

// Forward declarations
class Arduino_ESP32RGBPanel;
class Arduino_GC9107;
class GT911;

class DisplayManager {
public:
    static DisplayManager& getInstance() {
        static DisplayManager instance;
        return instance;
    }

    void begin();
    void setBrightness(uint8_t brightness);
    void update();
    
    // LVGL display and touch callbacks
    static void lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void lvgl_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
    
    // Display and touch getters
    Arduino_ESP32RGBPanel* getBus() { return bus; }
    Arduino_GC9107* getGfx() { return gfx; }
    GT911* getTouch() { return touch; }

private:
    DisplayManager();
    ~DisplayManager() = default;
    DisplayManager(const DisplayManager&) = delete;
    DisplayManager& operator=(const DisplayManager&) = delete;

    void initDisplay();
    void initTouch();
    void initLVGL();

    // Display and touch instances
    Arduino_ESP32RGBPanel *bus;
    Arduino_GC9107 *gfx;
    GT911 *touch;

    // LVGL display buffer
    static constexpr uint16_t screenWidth = SCREEN_WIDTH;
    static constexpr uint16_t screenHeight = SCREEN_HEIGHT;
    static constexpr uint16_t bufferSize = screenWidth * 40; // Adjust based on available memory
    lv_color_t *lv_display_buf1;
    lv_color_t *lv_display_buf2;
    lv_disp_draw_buf_t draw_buf;
    lv_disp_drv_t disp_drv;
    lv_indev_drv_t indev_drv;
    lv_disp_t *disp;
    lv_indev_t *indev_touch;

    // Touch state
    static bool touch_has_signal;
    static int16_t touch_last_x;
    static int16_t touch_last_y;
};

extern DisplayManager& displayManager;
