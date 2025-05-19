#include "DisplayManager.h"
#include <SPIFFS.h>
#include <FS.h>
#include <Wire.h>
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

bool DisplayManager::begin() {
    // Initialize display
    if (!initDisplay()) {
        Serial.println("Failed to initialize display");
        return false;
    }
    
    // Initialize touch
    if (!initTouch()) {
        Serial.println("Failed to initialize touch controller");
        // Continue anyway, touch might not be critical
    }
    
    // Initialize LVGL
    if (!initLVGL()) {
        Serial.println("Failed to initialize LVGL");
        return false;
    }
    
    Serial.println("DisplayManager initialized");
    return true;
}

bool DisplayManager::initDisplay() {
    // Initialize RGB panel with proper parameters for ESP32-S3
    Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
        40,    // DE pin
        41,    // VSYNC pin
        39,    // HSYNC pin
        42,    // PCLK pin
        45, 48, 47, 21, 14, // R0, R1, R2, R3, R4
        5, 6, 7, 15, 16, 4,  // G0, G1, G2, G3, G4, G5
        8, 3, 46, 9, 1,      // B0, B1, B2, B3, B4
        0, 8, 4, 8, 0, 8, 4, 8, 1, 16000000);
    
    if (!bus) {
        Serial.println("Failed to create RGB panel");
        return false;
    }
    
    // Create the display instance
    gfx = new Arduino_RGB_Display(SCREEN_WIDTH, SCREEN_HEIGHT, bus, 0, true);
    
    if (!gfx) {
        Serial.println("Failed to create display instance!");
        delete bus;
        return false;
    }
    
    // Initialize the display
    if (!gfx->begin()) {
        Serial.println("Failed to initialize display!");
        delete gfx;
        delete bus;
        return false;
    }
    
    // Set up backlight control
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);  // Turn on backlight
    
    // Set display orientation and clear screen
    gfx->setRotation(0);
    gfx->fillScreen(BLACK);
    
    // Initialize backlight with default brightness (80%)
    setBrightness(currentBrightness);
    
    Serial.println("Display initialized");
    
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
    // Initialize LVGL
    lv_init();
    
    // Allocate display buffer
    lv_display_buf1 = (lv_color_t*)ps_malloc(bufferSize * sizeof(lv_color_t));
    if (!lv_display_buf1) {
        Serial.println("Failed to allocate display buffer 1");
        return false;
    }
    
    // Optional: Allocate second buffer for double buffering
    lv_display_buf2 = (lv_color_t*)ps_malloc(bufferSize * sizeof(lv_color_t));
    if (!lv_display_buf2) {
        Serial.println("Failed to allocate display buffer 2, using single buffering");
        // Continue with single buffering
    }
    
    // Initialize the display buffer
    lv_disp_draw_buf_init(&draw_buf, lv_display_buf1, lv_display_buf2, bufferSize);
    
    // Initialize the display
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = gfx;
    disp = lv_disp_drv_register(&disp_drv);
    
    if (!disp) {
        Serial.println("Failed to register display driver");
        return false;
    }
    
    // Initialize the input device driver if touch is working
    if (isTouchWorking()) {
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lvgl_touchpad_read;
        indev_touch = lv_indev_drv_register(&indev_drv);
        
        if (!indev_touch) {
            Serial.println("Failed to register input device");
            // Continue without touch
        }
    } else {
        Serial.println("Touch not available, skipping input device initialization");
    }
    
    Serial.println("LVGL initialized");
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
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Update display if needed
    if (gfx) {
        // The display is updated in the LVGL flush callback
    }
}

// LVGL display flush callback
void DisplayManager::lvgl_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    Arduino_GC9107 *gfx = (Arduino_GC9107 *)disp->user_data;
    
    if (!gfx) {
        lv_disp_flush_ready(disp);
        return;
    }
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    gfx->startWrite();
    gfx->setAddrWindow(area->x1, area->y1, w, h);
    gfx->writePixels((uint16_t*)color_p, w * h);
    gfx->endWrite();
    
    // Tell LVGL the flush is done
    lv_disp_flush_ready(disp);
}

// LVGL touch read callback
void DisplayManager::lvgl_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    // Default to not touched
    data->state = LV_INDEV_STATE_REL;
    data->point.x = 0;
    data->point.y = 0;
    
    // Check if touch controller is available
    if (!touch_controller) {
        return;
    }
    
    // Read touch data
    touch_controller->read();
    
    if (touch_controller->isTouched && touch_controller->points[0].x > 0) {
        // Get the first touch point
        TP_Point p = touch_controller->points[0];
        // Map touch coordinates to display coordinates
        data->point.x = map(p.x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, SCREEN_WIDTH - 1);
        data->point.y = map(p.y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, SCREEN_HEIGHT - 1);
        data->state = LV_INDEV_STATE_PR;
        
        // Update last touch position for potential use elsewhere
        touch_last_x = data->point.x;
        touch_last_y = data->point.y;
        touch_has_signal = true;
        // Debug output
        // Serial.printf("Touch: x=%d, y=%d\n", data->point.x, data->point.y);
    } else {
        // If we had a signal before but don't have one now, use last known position
        if (touch_has_signal) {
            data->point.x = touch_last_x;
            data->point.y = touch_last_y;
            touch_has_signal = false;
        }
    }
}

