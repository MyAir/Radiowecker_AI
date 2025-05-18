#include "DisplayManager.h"
#include <Wire.h>
#include <Arduino_GFX.h>
#include <GT911.h>

// Initialize static members
bool DisplayManager::touch_has_signal = false;
int16_t DisplayManager::touch_last_x = 0;
int16_t DisplayManager::touch_last_y = 0;

// Global instance
DisplayManager& displayManager = DisplayManager::getInstance();

DisplayManager::DisplayManager() 
    : bus(nullptr), gfx(nullptr), touch(nullptr),
      lv_display_buf1(nullptr), lv_display_buf2(nullptr) {
}

void DisplayManager::begin() {
    initDisplay();
    initTouch();
    initLVGL();
    
    // Set initial brightness (0-255)
    setBrightness(128);
    
    // Clear screen
    gfx->fillScreen(BLACK);
    
    Serial.println("DisplayManager initialized");
}

void DisplayManager::initDisplay() {
    // Initialize RGB bus
    bus = new Arduino_ESP32RGBPanel(
        40,  // DE
        41,  // VSYNC
        39,  // HSYNC
        42,  // PCLK
        45,  // R0
        48,  // R1
        47,  // R2
        21,  // R3
        14,  // R4
        5,   // G0
        6,   // G1
        7,   // G2
        15,  // G3
        16,  // G4
        4,   // G5
        8,   // B0
        3,   // B1
        46,  // B2
        9,   // B3
        1,   // B4
        0,   // hsync_polarity
        8,   // hsync_front_porch
        4,   // hsync_pulse_width
        8,   // hsync_back_porch
        0,   // vsync_polarity
        8,   // vsync_front_porch
        4,   // vsync_pulse_width
        8,   // vsync_back_porch
        1,   // pclk_active_neg
        16000000  // prefer_speed
    );

    // Initialize display with GC9107 driver
    gfx = new Arduino_GC9107(bus);
    
    if (!gfx->begin()) {
        Serial.println("Failed to initialize display!");
        while (1) delay(1000);
    }
    
    gfx->fillScreen(BLACK);
    gfx->setTextColor(WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(10, 10);
    gfx->println("Display initialized");

    // Set backlight pin
    pinMode(TFT_BL, OUTPUT);
    ledcSetup(0, 5000, 8);  // 5kHz PWM, 8-bit resolution
    ledcAttachPin(TFT_BL, 0);
}

void DisplayManager::initTouch() {
    // Initialize I2C for touch controller
    Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
    
    // Initialize GT911 touch controller
    touch = new GT911();
    
    // Initialize touch controller
    if (!touch->begin(TOUCH_GT911_ADDR)) {
        Serial.println("Failed to initialize touch controller!");
        while (1) delay(1000);
    }
    
    // Set rotation if needed
    // touch->setRotation(TOUCH_GT911_ROTATION);
    Serial.println("Touch controller initialized");
}

void DisplayManager::initLVGL() {
    // Initialize LVGL
    lv_init();
    
    // Allocate display buffers
    lv_display_buf1 = (lv_color_t*)ps_malloc(bufferSize * sizeof(lv_color_t));
    lv_display_buf2 = (lv_color_t*)ps_malloc(bufferSize * sizeof(lv_color_t));
    
    if (!lv_display_buf1 || !lv_display_buf2) {
        Serial.println("Failed to allocate LVGL display buffers!");
        while (1) delay(1000);
    }
    
    // Initialize the display buffer
    lv_disp_draw_buf_init(&draw_buf, lv_display_buf1, lv_display_buf2, bufferSize);
    
    // Initialize the display
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp = lv_disp_drv_register(&disp_drv);
    
    // Initialize the touch input device
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touchpad_read;
    indev_touch = lv_indev_drv_register(&indev_drv);
    
    Serial.println("LVGL initialized");
}

void DisplayManager::setBrightness(uint8_t brightness) {
    ledcWrite(0, brightness);
}

void DisplayManager::update() {
    lv_timer_handler();
    lv_tick_inc(5); // Call this every 5ms
}

// LVGL display flush callback
void DisplayManager::lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    auto instance = &DisplayManager::getInstance();
    auto gfx = instance->getGfx();
    
    gfx->startWrite();
    gfx->setAddrWindow(area->x1, area->y1, w, h);
    gfx->writePixels((uint16_t*)color_p, w * h);
    gfx->endWrite();
    
    lv_disp_flush_ready(disp);
}

// LVGL touch read callback
void DisplayManager::lvgl_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    auto instance = &DisplayManager::getInstance();
    auto touch = instance->getTouch();
    
    // Read touch point
    if (touch->touched()) {
        uint8_t touches = touch->readTouchPoints();
        if (touches > 0) {
            // Get first touch point
            int16_t x = touch->points[0].x;
            int16_t y = touch->points[0].y;
            
            // Map touch coordinates if needed
            data->point.x = map(x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, SCREEN_WIDTH - 1);
            data->point.y = map(y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, SCREEN_HEIGHT - 1);
            data->state = LV_INDEV_STATE_PR;
            
            // Store last touch position
            touch_last_x = data->point.x;
            touch_last_y = data->point.y;
            touch_has_signal = true;
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    
    // If we had a signal before but don't have one now, use last known position
    if (touch_has_signal && data->state == LV_INDEV_STATE_REL) {
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
        touch_has_signal = false;
    }
}
