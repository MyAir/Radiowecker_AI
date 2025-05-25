#include "DisplayManager.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// Include ESP-IDF headers for low-level panel management
#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
#include "esp_lcd_panel_ops.h"  // For esp_lcd_panel_del
#include "esp_lcd_panel_rgb.h"  // For RGB panel-specific functions
#endif
#include <lvgl.h>
#include <TAMC_GT911.h>
#include <Arduino_GFX_Library.h>
#include "DisplayConfig.h"

// Global touch controller instance for LVGL
TAMC_GT911* DisplayManager::touch_controller = nullptr;

// Initialize static members
bool DisplayManager::touch_has_signal = false;
int16_t DisplayManager::touch_last_x = 0;
int16_t DisplayManager::touch_last_y = 0;

DisplayManager::DisplayManager() 
    : bus(nullptr), gfx(nullptr), touch(nullptr),
      lv_display_buf1(nullptr), lv_display_buf2(nullptr),
      disp(nullptr), indev_touch(nullptr),
      currentBrightness(80)  // Default to 80% brightness
{
}

void DisplayManager::performHardwareReset() {
    Serial.println("Performing hardware reset sequence...");
    
    // Step 1: Free all software resources first
    if (gfx != nullptr) {
        delete gfx;
        gfx = nullptr;
    }
    
    if (touch != nullptr) {
        delete touch;
        touch = nullptr;
    }
    
    // Step 2: Hardware reset on TFT_RST pin
    pinMode(TFT_RST, OUTPUT);
    digitalWrite(TFT_RST, LOW);  // Active low reset
    delay(100);                  // Hold in reset
    digitalWrite(TFT_RST, HIGH); // Release reset
    delay(100);                  // Wait for device to stabilize
    
    // Step 3: Reset all RGB panel pins to INPUT state
    // This helps clear any lingering state in the GPIO controller
    // Data pins
    const int dataPins[] = {
        // R pins
        45, 48, 47, 21, 14,
        // G pins
        5, 6, 7, 15, 16, 4,
        // B pins
        8, 3, 46, 9, 1
    };
    
    // Control pins
    const int controlPins[] = {40, 41, 39, 42}; // DE, VSYNC, HSYNC, PCLK
    
    // Reset all data pins to INPUT
    for (int i = 0; i < sizeof(dataPins) / sizeof(dataPins[0]); i++) {
        pinMode(dataPins[i], INPUT);
    }
    
    // Reset all control pins to INPUT
    for (int i = 0; i < sizeof(controlPins) / sizeof(controlPins[0]); i++) {
        pinMode(controlPins[i], INPUT);
    }
    
    // Step 4: Reset the backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);
    delay(50);
    
    // Step 5: Force reset of the ESP-IDF RGB LCD driver state
    // This is done by calling a low-level ESP-IDF function that forces the driver
    // to release any held panel slots
#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
    // Try to force-free any existing RGB panel resources at the ESP-IDF level
    // The ESP32-S3 has a limited number of RGB panel slots that might not be properly released
    Serial.println("Attempting to release any occupied RGB panel slots");
    
    // Create a dummy handle - we don't actually use this directly
    // But calling esp_lcd_panel_del with nullptr can help trigger internal cleanup
    esp_lcd_panel_handle_t dummy_handle = nullptr;
    
    // This is a speculative call that might help release resources
    // It's safe even if the handle is nullptr
    esp_lcd_panel_del(dummy_handle);
#endif
    
    // Step 6: Reset I2C for touch controller
    Wire.end();
    delay(50);
    Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
    
    // Step 7: Turn backlight back on
    digitalWrite(TFT_BL, HIGH);
    
    // Step 8: Final delay to ensure stable state
    delay(100);
    
    Serial.println("Hardware reset completed");
}

bool DisplayManager::begin() {
    // Force a delay at startup to ensure hardware is stable
    delay(200);
    
    // Following the initialization sequence from the working example
    
    // Step 1: Perform a complete hardware reset to ensure clean initialization
    performHardwareReset();
    
    // Step 2: Initialize touch controller first (like in the example)
    if (!initTouch()) {
        Serial.println("Failed to initialize touch controller");
        // Continue without touch
    }
    
    // Step 3: Initialize display hardware
    if (!initDisplay()) {
        Serial.println("Failed to initialize display");
        return false;
    }
    
    // Step 4: Set initial display brightness
    setBrightness(currentBrightness);
    
    // Step 5: Initialize LVGL (after display is ready)
    if (!initLVGL()) {
        Serial.println("Failed to initialize LVGL");
        // Continue without LVGL
        Serial.println("Continuing with basic display functionality");
    }
    
    Serial.println("DisplayManager initialized");
    return true;
}

bool DisplayManager::initDisplay() {
    // Follow the initialization approach from the working example
    
    // Clean up any existing resources first
    if (gfx != nullptr) {
        delete gfx;
        gfx = nullptr;
    }
    
    if (bus != nullptr) {
        delete bus;
        bus = nullptr;
    }
    
    // Create the RGB panel bus using the exact pin configuration from the working example
    bus = new Arduino_ESP32RGBPanel(
        40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
        45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
        5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
        8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
        0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
        0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
        1 /* pclk_active_neg */, 16000000 /* prefer_speed */);
    
    if (!bus) {
        Serial.println("Failed to create RGB panel bus");
        return false;
    }
    
    // Create the display instance
    gfx = new Arduino_RGB_Display(
        SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */, bus,
        0 /* rotation */, true /* auto_flush */);
    
    if (!gfx) {
        Serial.println("Failed to create display instance");
        delete bus;
        bus = nullptr;
        return false;
    }
    
    // Initialize the display
    if (!gfx->begin()) {
        Serial.println("Failed to initialize display");
        delete gfx;
        gfx = nullptr;
        delete bus;
        bus = nullptr;
        return false;
    }
    
    // Clear the screen with black color
    gfx->fillScreen(BLACK);
    
    // Configure backlight pin
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);  // Turn on backlight (active LOW)
    
    Serial.println("Display initialized successfully");
    return true;
}

bool DisplayManager::initTouch() {
    // Reset the touch controller
    pinMode(TOUCH_GT911_RST, OUTPUT);
    digitalWrite(TOUCH_GT911_RST, LOW);
    delay(100);
    digitalWrite(TOUCH_GT911_RST, HIGH);
    delay(1000);
    
    // Initialize I2C for touch controller
    Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL, 400000); // 400kHz I2C clock
    
    // Initialize TAMC_GT911 touch controller
    touch = new TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    if (!touch) {
        Serial.println("Failed to create touch controller instance");
        return false;
    }
    
    // Initialize touch controller
    touch->begin();
    touch->setRotation(TOUCH_ROTATION);
    
    // Set the global touch controller instance for LVGL
    touch_controller = touch;
    
    Serial.println("Touch controller initialized");
    Serial.printf("  Resolution: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    Serial.printf("  Rotation: %d\n", TOUCH_ROTATION);
    return true;
}

bool DisplayManager::initLVGL() {
    // Initialize LVGL following the pattern from the working example
    lv_init();
    
    // Get screen dimensions
    uint32_t screenWidth = gfx->width();
    uint32_t screenHeight = gfx->height();
    
    // Allocate display buffer using the same method as the working example
    // This uses specific memory allocation flags that work well with ESP32-S3
    lv_display_buf1 = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * 10, 
                                                  MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    
    if (!lv_display_buf1) {
        Serial.println("LVGL display buffer allocation failed!");
        return false;
    }
    
    // Initialize the display buffer
    lv_disp_draw_buf_init(&draw_buf, lv_display_buf1, NULL, screenWidth * 10);
    
    // Initialize the display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp = lv_disp_drv_register(&disp_drv);
    
    // Initialize touch input if available
    if (touch_initialized) {
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lvgl_touchpad_read;
        indev_touch = lv_indev_drv_register(&indev_drv);
    } else {
        Serial.println("Touch not available, skipping input device initialization");
    }
    
    Serial.println("LVGL initialized successfully");
    return true;
}

void DisplayManager::setBrightness(uint8_t level) {
    currentBrightness = level;
    // Map 0-100 to 0-255 for the backlight control
    uint8_t pwm = map(currentBrightness, 0, 100, 0, 255);
    
    // Configure LEDC for backlight control
    static bool ledc_configured = false;
    if (!ledc_configured) {
        ledcSetup(0, 5000, 8);  // 5kHz PWM, 8-bit resolution
        ledcAttachPin(TFT_BL, 0);
        ledc_configured = true;
    }
    
    // Set the brightness
    ledcWrite(0, pwm);
    
    Serial.printf("Display brightness set to %d%% (PWM: %d)\n", currentBrightness, pwm);
}

void DisplayManager::update() {
    // Handle LVGL updates in a simple, single-threaded way similar to the working example
    static uint32_t lastLvglUpdate = 0;
    uint32_t now = millis();
    
    // Process LVGL tasks periodically (5ms like in the example)
    if (now - lastLvglUpdate > 5) {
        lv_timer_handler(); // Let LVGL do its work
        lastLvglUpdate = now;
    }
    
    // Make sure the display is refreshed
    if (gfx) {
        gfx->flush();
    }
}

// LVGL display flush callback
void DisplayManager::lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    // Get instance for the callback
    DisplayManager &dm = DisplayManager::getInstance();
    
    if (dm.gfx != nullptr) {
        uint32_t w = (area->x2 - area->x1 + 1);
        uint32_t h = (area->y2 - area->y1 + 1);
        
        // Use the same drawing method as the working example
        #if (LV_COLOR_16_SWAP != 0)
        dm.gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
        #else
        dm.gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
        #endif
        
        // Notify lvgl that flush is done
        lv_disp_flush_ready(disp);
    }
}

// LVGL touch read callback - based on the working example
void DisplayManager::lvgl_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    // Get instance for the callback
    DisplayManager &dm = DisplayManager::getInstance();
    
    // Following the pattern from the working example touch.h
    if (dm.touch && dm.touch_initialized) {
        // Check for touch input
        dm.touch->read();
        
        if (dm.touch->isTouched) {
            // Touch detected
            data->state = LV_INDEV_STATE_PR;
            
            // Get first touch point coordinates and map them to screen coordinates
            int16_t raw_x = dm.touch->points[0].x;
            int16_t raw_y = dm.touch->points[0].y;
            
            // Map touch coordinates to screen coordinates (similar to the example)
            touch_last_x = map(raw_x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, SCREEN_WIDTH - 1);
            touch_last_y = map(raw_y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, SCREEN_HEIGHT - 1);
            
            // Set the touch coordinates
            data->point.x = touch_last_x;
            data->point.y = touch_last_y;
            
            // Update the signal flag
            touch_has_signal = true;
        } else if (touch_has_signal) {
            // Touch released
            data->state = LV_INDEV_STATE_REL;
            touch_has_signal = false;
        } else {
            // No touch
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        // Touch not available
        data->state = LV_INDEV_STATE_REL;
    }
}

