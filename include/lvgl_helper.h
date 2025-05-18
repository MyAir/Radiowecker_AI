#pragma once

#include <Arduino_GFX_Library.h>
#include <lvgl.h>

// Display dimensions
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// LVGL display and input device handles
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// Display buffer
static lv_color_t *disp_draw_buf = nullptr;
static lv_color_t *disp_draw_buf2 = nullptr;

// Forward declarations
void lvgl_display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void lvgl_touchpad_read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

// Initialize LVGL with Arduino_GFX
void lvgl_init(Arduino_GFX *gfx, int touch_sda = -1, int touch_scl = -1, int touch_rst = -1, uint8_t touch_addr = 0) {
    // Initialize LVGL
    lv_init();

    // Initialize display buffer
    size_t buf_size = SCREEN_WIDTH * 40; // Smaller buffer for memory efficiency
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!disp_draw_buf) {
        Serial.println("LVGL: Failed to allocate display buffer");
        return;
    }

    // Optional second buffer (can be NULL for single buffering)
    disp_draw_buf2 = nullptr;
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, disp_draw_buf2, buf_size);

    // Initialize the display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = lvgl_display_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = gfx;
    lv_disp_drv_register(&disp_drv);

    // Initialize touch if pins are provided
    if (touch_sda >= 0 && touch_scl >= 0) {
        Wire.begin(touch_sda, touch_scl);
        
        if (touch_rst >= 0) {
            pinMode(touch_rst, OUTPUT);
            digitalWrite(touch_rst, LOW);
            delay(10);
            digitalWrite(touch_rst, HIGH);
            delay(10);
        }

        // Initialize the input device driver
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = lvgl_touchpad_read_cb;
        indev_drv.user_data = gfx;
        lv_indev_drv_register(&indev_drv);
    }
}

// Display flush callback
void lvgl_display_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    Arduino_GFX *gfx = (Arduino_GFX *)disp->user_data;
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    gfx->startWrite();
    gfx->setAddrWindow(area->x1, area->y1, w, h);
    gfx->writePixels((uint16_t*)color_p, w * h);
    gfx->endWrite();
    
    lv_disp_flush_ready(disp);
}

// Touchpad read callback
void lvgl_touchpad_read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    // This needs to be implemented based on your touch controller
    // For now, we'll just set it to not pressed
    data->state = LV_INDEV_STATE_RELEASED;
    data->point.x = 0;
    data->point.y = 0;
}

// LVGL task handler - call this in your main loop
void lvgl_task_handler() {
    lv_task_handler();
}

// Set screen brightness (0-255)
void set_screen_brightness(uint8_t brightness) {
    // Implement this based on your display's backlight control
    analogWrite(44, brightness); // Assuming pin 44 is backlight control
}
