#include "DisplayManager.h"
#include "UIManager.h"
#include "Globals.h"
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

// Forward declaration of UIManager class
class UIManager;

// Include ESP-IDF headers for low-level panel management
#if defined(ESP32) && (CONFIG_IDF_TARGET_ESP32S3)
#include "esp_lcd_panel_ops.h"  // For esp_lcd_panel_del
#include "esp_lcd_panel_rgb.h"  // For RGB panel-specific functions
#endif
#include <lvgl.h>
// Use our SafeTouchController instead of TAMC_GT911
#include "SafeTouchController.h"
#include <Arduino_GFX_Library.h>
#include "DisplayConfig.h"

// Global touch controller instance for LVGL
SafeTouchController* DisplayManager::touch_controller = nullptr;

// Initialize static members
lv_color_t* DisplayManager::lv_display_buf1 = nullptr;
lv_color_t* DisplayManager::lv_display_buf2 = nullptr;
bool DisplayManager::touch_has_signal = false;
int16_t DisplayManager::touch_last_x = 0;
int16_t DisplayManager::touch_last_y = 0;

DisplayManager::DisplayManager() 
    : bus(nullptr), gfx(nullptr), safe_touch(nullptr),
      disp(nullptr), indev_touch(nullptr),
      currentBrightness(80),  // Default to 80% brightness
      touch_initialized(false)
{
}

void DisplayManager::performHardwareReset() {
    Serial.println("Performing hardware reset sequence...");
    
    // Reset the touch controller in a simpler way
    if (TOUCH_GT911_RST >= 0) {
        pinMode(TOUCH_GT911_RST, OUTPUT);
        digitalWrite(TOUCH_GT911_RST, LOW);
        delay(20);
        digitalWrite(TOUCH_GT911_RST, HIGH);
        Serial.println("Touch controller reset pin cycled");
    }
    delay(100);
    
    // Reset I2C for touch controller using our more robust I2C management function
    resetI2C(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
    
    // Allow time for I2C to stabilize
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
    touch_initialized = false; // Reset flag
    if (!initTouch()) {
        Serial.println("Failed to initialize touch controller");
        // Continue without touch
    } else {
        touch_initialized = true;
        Serial.println("Touch controller initialized successfully");
    }
    
    // Step 3: Initialize display
    if (!initDisplay()) {
        Serial.println("Failed to initialize display");
        return false;
    }
    
    // Step 4: Initialize LVGL
    if (!initLVGL()) {
        Serial.println("Failed to initialize LVGL");
        return false;
    }
    
    // Step 5: Set the brightness to a default value (if backlight control is available)
    if (TFT_BL >= 0 && TFT_BL < 48) {
        setBrightness(currentBrightness);
    } else {
        Serial.println("Backlight control disabled - skipping brightness setting");
    }
    
    // Log touch status for debugging
    Serial.printf("Touch status: %s\n", touch_initialized ? "ENABLED" : "DISABLED");
    if (touch_initialized && safe_touch) {
        Serial.printf("Touch controller addr: 0x%p\n", safe_touch);
    }
    
    Serial.println("DisplayManager initialized successfully");
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
    
    // Configure backlight pin only if it's valid
    if (TFT_BL >= 0 && TFT_BL < 48) {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, LOW);  // Turn on backlight (active LOW)
        Serial.printf("Backlight pin %d configured\n", TFT_BL);
    } else {
        Serial.println("Backlight control disabled - skipping pin configuration");
    }
    
    Serial.println("Display initialized successfully");
    return true;
}

bool DisplayManager::initTouch() {
    Serial.println("Initializing touch controller with safe implementation...");
    
    // Free any existing touch instances
    if (safe_touch != nullptr) {
        delete safe_touch;
        safe_touch = nullptr;
    }
    
    if (touch_controller != nullptr) {
        delete touch_controller;
        touch_controller = nullptr;
    }
    
    // Create our safer touch controller instances
    // One for the DisplayManager instance and one static for callbacks
    safe_touch = new SafeTouchController(
        TOUCH_GT911_SDA, 
        TOUCH_GT911_SCL, 
        TOUCH_GT911_RST, 
        SCREEN_WIDTH, 
        SCREEN_HEIGHT);
    
    // Create the static instance for LVGL callbacks
    touch_controller = new SafeTouchController(
        TOUCH_GT911_SDA, 
        TOUCH_GT911_SCL, 
        TOUCH_GT911_RST, 
        SCREEN_WIDTH, 
        SCREEN_HEIGHT);
    
    if (!safe_touch || !touch_controller) {
        Serial.println("Failed to allocate memory for safe touch controller");
        touch_initialized = false;
        return false;
    }
    
    // Initialize the touch controllers using our safe wrapper
    bool success1 = safe_touch->begin();
    bool success2 = touch_controller->begin();
    bool success = success1 && success2;
    
    if (success) {
        Serial.println("Safe touch controller initialized successfully");
        touch_initialized = true;
        return true;
    } else {
        Serial.println("Failed to initialize safe touch controller");
        touch_initialized = false;
        return false;
    }
}

bool DisplayManager::initLVGL() {
    static bool lvgl_initialized = false;
    if (lvgl_initialized) {
        Serial.println("LVGL already initialized, skipping");
        return true;
    }
    
    Serial.println("Initializing LVGL with minimal configuration");
    
    // Initialize LVGL
    lv_init();
    
    // Safety check for display
    if (!gfx) {
        Serial.println("ERROR: Display not initialized");
        return false;
    }
    
    // Get screen dimensions
    uint32_t screenWidth = gfx->width();
    uint32_t screenHeight = gfx->height();
    
    Serial.printf("Screen dimensions: %dx%d\n", screenWidth, screenHeight);
    
    // Use a larger buffer for better performance (1/10th of screen size)
    size_t buf_size = (screenWidth * screenHeight) / 10;
    
    // Allocate display buffer (double buffering for smoother updates)
    lv_display_buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_display_buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    
    if (!lv_display_buf1 || !lv_display_buf2) {
        Serial.println("ERROR: Failed to allocate LVGL display buffers!");
        if (lv_display_buf1) free(lv_display_buf1);
        if (lv_display_buf2) free(lv_display_buf2);
        lv_display_buf1 = nullptr;
        lv_display_buf2 = nullptr;
        return false;
    }
    
    // Initialize display buffer with double buffering
    lv_disp_draw_buf_init(&draw_buf, lv_display_buf1, lv_display_buf2, buf_size);
    
    // Initialize display driver with optimized settings
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = false;   // Use partial updates
    disp_drv.direct_mode = false;    // Use buffered mode
    disp_drv.antialiasing = false;   // Disable antialiasing for better performance
    disp_drv.sw_rotate = 0;          // No software rotation
    disp_drv.screen_transp = 0;      // No transparency
    
    // Register the display driver
    disp = lv_disp_drv_register(&disp_drv);
    
    // Set a faster refresh rate (using the correct API for this LVGL version)
    // The refresh rate is controlled by the lv_timer_handler() call frequency in the update() method
    
    // Touch input only if available
    if (touch_initialized && safe_touch) {
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lvgl_touchpad_read;
        indev_touch = lv_indev_drv_register(&indev_drv);
        Serial.println("Touch input registered with LVGL");
    }
    
    // Use the most minimal theme possible
    lv_theme_t* theme = lv_theme_default_init(disp, 
                                        lv_palette_main(LV_PALETTE_BLUE),
                                        lv_palette_main(LV_PALETTE_BLUE_GREY),
                                        true,  // Dark theme
                                        LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, theme);
    
    // Set a reasonable default screen refresh rate
    // Note: lv_timer_get_next() requires an argument in this version of LVGL
    // We'll skip this configuration since it's causing compilation errors
    
    lvgl_initialized = true;
    Serial.println("LVGL minimal initialization complete");
    return true;
}

void DisplayManager::setBrightness(uint8_t brightness) {
    // Set the backlight brightness using PWM
    // The ESP32 uses values from 0-255 for PWM
    if (brightness > 255) brightness = 255;
    
    currentBrightness = brightness;
    
    // Only proceed if backlight pin is valid and in the proper range for ESP32-S3
    // ESP32-S3 has GPIO pins 0-48, and pin must be positive
    if (TFT_BL < 0 || TFT_BL >= 48) {
        Serial.println("Cannot set brightness - backlight pin is invalid or disabled");
        return;
    }
    
    // Set up the PWM channel if not already done
    if (!pwmSetup) {
        // Try-catch to handle potential hardware issues
        try {
            ledcSetup(BACKLIGHT_PWM_CHANNEL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);
            ledcAttachPin(TFT_BL, BACKLIGHT_PWM_CHANNEL);
            pwmSetup = true;
            Serial.printf("PWM setup completed for backlight pin %d\n", TFT_BL);
        } catch (...) {
            Serial.printf("Failed to set up PWM for backlight pin %d\n", TFT_BL);
            return;
        }
    }
    
    // Calculate PWM value (0-255)
    uint32_t pwmValue = map(brightness, 0, 100, 0, 255);
    
    // Write the PWM value to the backlight pin
    try {
        ledcWrite(BACKLIGHT_PWM_CHANNEL, pwmValue);
        Serial.printf("Display brightness set to %d%% (PWM: %d)\n", brightness, pwmValue);
    } catch (...) {
        Serial.println("Error setting display brightness");
    }
}

void DisplayManager::update() {
    // Process touch events if touch is initialized
    if (touch_initialized && safe_touch) {
        static uint32_t lastTouchCheck = 0;
        static bool lastTouchState = false;
        static uint32_t touchStartTime = 0;
        static uint32_t touchEndTime = 0;
        
        // Handle touch input if touch controller is active
        uint32_t now = millis();
        
        // Check touch at a reasonable rate (every 20ms = 50Hz)
        if (now - lastTouchCheck >= 20) {
            // Try to read touch data - this uses our safer implementation
            safe_touch->read();
            lastTouchCheck = now;
            
            // Get current touch state
            bool currentTouchState = safe_touch->isTouched();
            
            // Process touch state changes
            if (currentTouchState != lastTouchState) {
                if (currentTouchState) {
                    // Touch started
                    touchStartTime = now;
                    Serial.printf("Touch started at X:%d Y:%d\n", safe_touch->x, safe_touch->y);
                    
                    // Update static variables for LVGL
                    touch_last_x = safe_touch->x;
                    touch_last_y = safe_touch->y;
                    touch_has_signal = true;
                    
                    // Update the global touch time to track user interaction for screen timeout
                    g_last_touch_time = now;
                } else {
                    // Touch ended
                    touchEndTime = now;
                    uint32_t touchDuration = touchEndTime - touchStartTime;
                    touch_has_signal = false;
                    
                    // Detect tap (short touch duration)
                    if (touchDuration < 300) {
                        Serial.printf("Tap detected at X:%d Y:%d (duration: %d ms)\n", touch_last_x, touch_last_y, touchDuration);
                    }
                }
                
                // Update last touch state
                lastTouchState = currentTouchState;
            }
        }
    }
    
    // Print memory usage every 30 seconds
    static uint32_t lastMemoryPrint = 0;
    uint32_t now = millis();
    if (now - lastMemoryPrint >= 30000) {
        Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
        lastMemoryPrint = now;
    }
    
    // Call LVGL task handler once at the end of the update
    lv_task_handler();
}

bool DisplayManager::isTouched() {
    if (!safe_touch || !touch_initialized) {
        return false;
    }
    
    // Use our new safe touch controller's isTouched method
    return safe_touch->isTouched();
}

// LVGL display flush callback
void DisplayManager::lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    // Get the DisplayManager instance for access to the gfx object
    DisplayManager& dm = DisplayManager::getInstance();
    Arduino_GFX* gfx = dm.getGfx();
    
    if (!gfx) {
        Serial.println("ERROR: gfx is null in lvgl_flush_cb");
        lv_disp_flush_ready(disp); // Signal LVGL that flush is done
        return;
    }
    
    // Calculate width and height of area to update
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    // Debug output for large updates (helps track screen redraws)
    if (w > 200 && h > 200) {
        Serial.printf("Flushing large area: (%d,%d) to (%d,%d) - %dx%d pixels\n", 
                     area->x1, area->y1, area->x2, area->y2, w, h);
    }
    
    // Use the Arduino_GFX to draw directly using the bitmap methods
    #if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    #else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
    #endif
    
    // Tell LVGL the flush is done
    lv_disp_flush_ready(disp);
}

// LVGL touch read callback - simplified for reliability
void DisplayManager::lvgl_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    static bool reported_touch = false;
    
    // Using the static touch_controller instead of instance members
    // Check if touch controller is properly initialized
    if (!touch_controller) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    
    // Read the latest touch state
    touch_controller->read();
    
    // Check if touch is active
    if (touch_controller->isTouched()) {
        // Set the touch coordinates
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_controller->x;
        data->point.y = touch_controller->y;
        
        // Update the static variables for other parts of the code
        touch_has_signal = true;
        touch_last_x = touch_controller->x;
        touch_last_y = touch_controller->y;
        
        // Only log once per touch to avoid spamming the console
        if (!reported_touch) {
            Serial.printf("LVGL touch event: PRESSED at (%d,%d)\n", touch_last_x, touch_last_y);
            reported_touch = true;
        }
    } else {
        // Touch released
        data->state = LV_INDEV_STATE_REL;
        touch_has_signal = false;
        
        // Only log release once
        if (reported_touch) {
            Serial.println("LVGL touch event: RELEASED");
            reported_touch = false;
        }
    }
}
