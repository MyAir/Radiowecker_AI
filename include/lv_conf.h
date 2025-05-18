#ifndef LV_CONF_H
#define LV_CONF_H

/* clang-format off */
#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

#define LV_COLOR_DEPTH 16

/*====================
   MEMORY SETTINGS
 *====================*/
#define LV_MEM_SIZE (128 * 1024U)

/*====================
   HAL SETTINGS
 *====================*/
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/*======================
 * FEATURE CONFIGURATION
 *======================*/
#define LV_USE_PERF_MONITOR 1
#define LV_USE_MEM_MONITOR 1

/*-------------
 * Logging
 *-----------*/
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
    #define LV_LOG_PRINTF 1
#endif

/*-------------
 * Themes
 *-----------*/
#define LV_USE_THEME_DEFAULT 1

/*-------------
 * Widgets
 *-----------*/
#define LV_USE_LABEL      1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CHECKBOX   1
#define LV_USE_SWITCH     1
#define LV_USE_SLIDER     1
#define LV_USE_DROPDOWN   1
#define LV_USE_ROLLER     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1
#define LV_USE_TABVIEW    1
#define LV_USE_WIN        1
#define LV_USE_PAGE       1
#define LV_USE_CALENDAR   1
#define LV_USE_CHART      1
#define LV_USE_LINE       1
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_GAUGE      1
#define LV_USE_IMGBTN     1
#define LV_USE_IMGBTN     1
#define LV_USE_KEYBOARD   1
#define LV_USE_LIST       1
#define LV_USE_MSGBOX     1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TILEVIEW   1

/*-------------
 * Extra themes
 *-----------*/
#define LV_USE_THEME_MATERIAL 1

/*-------------
 * Fonts
 *-----------*/
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_48 1
#define LV_FONT_MONTSERRAT_72 1

/*-------------
 * Others
 *-----------*/
#define LV_USE_ANIMATION 1
#define LV_USE_GROUP     1
#define LV_USE_FILESYSTEM 1

/*-------------
 * GPU
 *-----------*/
#define LV_USE_GPU_STM32_DMA2D 0

#endif /*LV_CONF_H*/
