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
    
    // Reset the touch controller in a simpler way
    if (TOUCH_GT911_RST >= 0) {
        pinMode(TOUCH_GT911_RST, OUTPUT);
        digitalWrite(TOUCH_GT911_RST, LOW);
        delay(20);
        digitalWrite(TOUCH_GT911_RST, HIGH);
        Serial.println("Touch controller reset pin cycled");
    }
    delay(100);
    
    // Reset I2C for touch controller with more delay to ensure stability
    Wire.end();
    delay(100); // Longer delay
    Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
    delay(100); // Longer delay after starting I2C
    
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
    if (touch_initialized && touch) {
        Serial.printf("Touch controller addr: 0x%p\n", touch);
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
    
    // Configure backlight pin
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);  // Turn on backlight (active LOW)
    
    Serial.println("Display initialized successfully");
    return true;
}

bool DisplayManager::initTouch() {
    Serial.println("Initializing touch controller with safe approach...");
    
    // Free any existing touch instance
    if (touch != nullptr) {
        delete touch;
        touch = nullptr;
    }
    
    // Add longer delays to ensure hardware stability
    delay(100);
    
    // Try to create the touch controller
    try {
        touch = new TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST, SCREEN_WIDTH, SCREEN_HEIGHT);
        
        if (!touch) {
            Serial.println("Failed to allocate memory for touch controller");
            return false;
        }
        
        // Extra delay to ensure hardware is ready
        delay(100);
        
        // Call begin() - note that this returns void, not bool
        touch->begin();
        delay(100); // Give it time to initialize
        
        // Manually check if touch is working by reading a value
        touch->read();
        if (touch->touches == 0) { // If reading succeeded, we should at least have valid touch count
            Serial.println("Touch controller initialized successfully");
            touch_initialized = true;
            Serial.printf("  Resolution: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
            Serial.printf("  Rotation: %d\n", TOUCH_ROTATION);
            return true;
        } else {
            Serial.println("Touch controller might not be working properly");
            // Continue anyway as this is a soft failure
            touch_initialized = true;
            return true;
        }
    } catch (...) {
        Serial.println("Exception during touch initialization - continuing without touch");
        if (touch) {
            delete touch;
            touch = nullptr;
        }
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
    
    // Use a very small buffer size for stability
    size_t buf_size = screenWidth * 10; 
    
    // Single buffer only for maximum stability
    lv_display_buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
    if (!lv_display_buf1) {
        Serial.println("ERROR: Failed to allocate LVGL display buffer!");
        return false;
    }
    
    // Initialize display buffer (single buffer)
    lv_disp_draw_buf_init(&draw_buf, lv_display_buf1, NULL, buf_size);
    
    // Initialize display driver with absolute minimum settings
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = true;    // More stable
    disp_drv.direct_mode = false;    // More stable
    disp_drv.antialiasing = false;   // Disable antialiasing
    disp = lv_disp_drv_register(&disp_drv);
    
    // Touch input only if available
    if (touch_initialized && touch) {
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
    
    // Only proceed if backlight pin is valid
    if (TFT_BL < 0 || TFT_BL >= 48) {
        Serial.println("Cannot set brightness - backlight pin is invalid");
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
        Serial.println("Failed to set brightness via PWM");
    }
}

void DisplayManager::update() {
    static uint32_t lastLvglUpdate = 0;
    static uint32_t lastTouchCheck = 0;
    static uint32_t lastDebugTime = 0;
    uint32_t now = millis();
    
    // Minimal debug output every 10 seconds to verify operation
    if (now - lastDebugTime > 10000) {
        Serial.println("DisplayManager update running");
        lastDebugTime = now;
    }
    
    // First priority: Process LVGL timers at a steady rate (20Hz - more stable)
    if (now - lastLvglUpdate > 50) {
        // Only process if the display is initialized
        if (gfx && lv_disp_get_default()) {
            // Process LVGL tasks
            lv_timer_handler();
            
            // Force a simple invalidation occasionally to ensure UI updates
            static uint8_t refresh_counter = 0;
            if (++refresh_counter >= 10) { // Every ~10th update (500ms)
                lv_obj_invalidate(lv_scr_act());
                refresh_counter = 0;
            }
        }
        lastLvglUpdate = now;
    }
    
    // Check touch at a lower frequency (10Hz) to reduce processing load
    if (now - lastTouchCheck > 100 && touch != nullptr && touch_initialized) {
        // Read touch data
        touch->read();
        
        // Process touch events
        if (touch->isTouched) {
            int16_t touchX = touch->points[0].x;
            int16_t touchY = touch->points[0].y;
            
            // Update touch coordinates for LVGL
            touch_last_x = touchX;
            touch_last_y = touchY;
            
            // Only set touch signal if not already set
            if (!touch_has_signal) {
                touch_has_signal = true;
                Serial.printf("Touch: x=%d, y=%d\n", touchX, touchY);
            }
        } else if (touch_has_signal) {
            // Touch released
            touch_has_signal = false;
            Serial.println("Touch released");
        }
        
        lastTouchCheck = now;
    }
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

// LVGL touch read callback - based on the working example
void DisplayManager::lvgl_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    static bool reported_touch = false;
    
    if (touch_has_signal) {
        // Set the touch coordinates from the static variables
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch_last_x;
        data->point.y = touch_last_y;
        
        // Only log once per touch to avoid spamming the console
        if (!reported_touch) {
            Serial.printf("LVGL touch event: PRESSED at (%d,%d)\n", touch_last_x, touch_last_y);
            reported_touch = true;
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
        
        // Only log release once
        if (reported_touch) {
            Serial.println("LVGL touch event: RELEASED");
            reported_touch = false;
        }
    }
}
