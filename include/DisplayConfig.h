#pragma once

// Display configuration for MaTouch ESP32-S3 Parallel TFT 4.3"
// Using Arduino_GFX_Library

// Display settings
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// Parallel interface pins
#define TFT_RD -1    // Not used
#define TFT_WR 40    // PCLK
#define TFT_RS 45    // DC
#define TFT_CS 42    // CS
#define TFT_RST -1   // Use -1 if not connected
#define TFT_D0 8     // Data pins
#define TFT_D1 3
#define TFT_D2 46
#define TFT_D3 9
#define TFT_D4 1
#define TFT_D5 5
#define TFT_D6 6
#define TFT_D7 7

// Backlight control
#define TFT_BL 44    // Backlight control pin

// Touch screen configuration (GT911)
#define TOUCH_GT911_SCL 18
#define TOUCH_GT911_SDA 17
#define TOUCH_GT911_INT -1  // Not used with TAMC_GT911
#define TOUCH_GT911_RST 38  // Reset pin for GT911
#define TOUCH_GT911_ADDR 0x14  // Default I2C address for GT911

// Touch mapping for TAMC_GT911 touch controller
// These values map the raw touch coordinates to the display coordinates
// You may need to adjust these based on your specific display and touch panel
#define TOUCH_MAP_X1 0      // Minimum X value from touch controller
#define TOUCH_MAP_X2 800    // Maximum X value from touch controller
#define TOUCH_MAP_Y1 0      // Minimum Y value from touch controller
#define TOUCH_MAP_Y2 480    // Maximum Y value from touch controller

// Touch rotation (0-3)
#define TOUCH_ROTATION 0

// LVGL configuration - Only define if not already defined by LVGL
#ifndef LV_HOR_RES_MAX
#define LV_HOR_RES_MAX SCREEN_WIDTH
#endif

#ifndef LV_VER_RES_MAX
#define LV_VER_RES_MAX SCREEN_HEIGHT
#endif

// Double buffer size (adjust based on available PSRAM)
#ifndef LV_COLOR_DEPTH
#define LV_COLOR_DEPTH 16
#endif

#ifndef LV_COLOR_16_SWAP
#define LV_COLOR_16_SWAP 1
#endif

// Enable if you have PSRAM
#ifndef LV_USE_DRAW_SW
#define LV_USE_DRAW_SW 1
#endif

// Memory configuration - Only define if not already defined by LVGL
#ifndef LV_MEM_SIZE
#define LV_MEM_SIZE (64 * 1024)  // 64KB for LVGL
#endif

#ifndef LV_MEM_CUSTOM
#define LV_MEM_CUSTOM 1
#endif

// Theme configuration - Only define if not already defined by LVGL
#ifndef LV_USE_THEME_DEFAULT
#define LV_USE_THEME_DEFAULT 1
#endif

#ifndef LV_THEME_DEFAULT_DARK
#define LV_THEME_DEFAULT_DARK 0
#endif

#ifndef LV_THEME_DEFAULT_TRANSITION_TIME
#define LV_THEME_DEFAULT_TRANSITION_TIME 0
#endif

#ifndef LV_USE_THEME_MONO
#define LV_USE_THEME_MONO 0
#endif

// Font configuration - only define if not already defined by LVGL
#ifndef LV_FONT_MONTSERRAT_8
#define LV_FONT_MONTSERRAT_8 1
#endif
#ifndef LV_FONT_MONTSERRAT_10
#define LV_FONT_MONTSERRAT_10 1
#endif
#ifndef LV_FONT_MONTSERRAT_12
#define LV_FONT_MONTSERRAT_12 1
#endif
#ifndef LV_FONT_MONTSERRAT_14
#define LV_FONT_MONTSERRAT_14 1
#endif
#ifndef LV_FONT_MONTSERRAT_16
#define LV_FONT_MONTSERRAT_16 1
#endif
#ifndef LV_FONT_MONTSERRAT_18
#define LV_FONT_MONTSERRAT_18 1
#endif
#ifndef LV_FONT_MONTSERRAT_20
#define LV_FONT_MONTSERRAT_20 1
#endif
#ifndef LV_FONT_MONTSERRAT_22
#define LV_FONT_MONTSERRAT_22 1
#endif
#ifndef LV_FONT_MONTSERRAT_24
#define LV_FONT_MONTSERRAT_24 1
#endif
#ifndef LV_FONT_MONTSERRAT_28
#define LV_FONT_MONTSERRAT_28 1
#endif
#ifndef LV_FONT_MONTSERRAT_32
#define LV_FONT_MONTSERRAT_32 1
#endif
#ifndef LV_FONT_MONTSERRAT_36
#define LV_FONT_MONTSERRAT_36 1
#endif
#ifndef LV_FONT_MONTSERRAT_40
#define LV_FONT_MONTSERRAT_40 1
#endif
#ifndef LV_FONT_MONTSERRAT_44
#define LV_FONT_MONTSERRAT_44 1
#endif
#ifndef LV_FONT_MONTSERRAT_48
#define LV_FONT_MONTSERRAT_48 1
#endif

// Enable extra features
#define LV_USE_ANIMATION 1
#define LV_USE_SHADOW 1
#define LV_USE_IMG_TRANSFORM 1
#define LV_USE_OBJ_PROPERTY 1
#define LV_USE_GROUP 1
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMAGE 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1
#define LV_USE_TABVIEW 1
#define LV_USE_TILEVIEW 1
#define LV_USE_WIN 1

// Enable styles
#define LV_USE_STYLE 1

// Enable file system
#ifndef LV_USE_FS_POSIX
#define LV_USE_FS_POSIX 1
#endif

#ifndef LV_FS_POSIX_LETTER
#define LV_FS_POSIX_LETTER 'S'  // 'S' for SD card
#endif

#ifndef LV_FS_POSIX_PATH
#define LV_FS_POSIX_PATH ""
#endif

#ifndef LV_FS_POSIX_CACHE_SIZE
#define LV_FS_POSIX_CACHE_SIZE 1
#endif

#ifndef LV_USE_FS_FATFS
#define LV_USE_FS_FATFS 0
#endif

#ifndef LV_USE_FS_LITTLEFS
#define LV_USE_FS_LITTLEFS 0
#endif

#ifndef LV_USE_FS_SPIFFS
#define LV_USE_FS_SPIFFS 1
#endif

// Enable image decoders
#define LV_IMG_CF_INDEXED 1
#define LV_IMG_CF_ALPHA 1

// Enable demo widgets
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
