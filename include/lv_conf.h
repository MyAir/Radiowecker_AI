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
    #define LV_LOG_LEVEL 3 /* LV_LOG_LEVEL_INFO */
    #define LV_LOG_PRINTF 1
#endif

/* Assertion settings */
#define LV_USE_ASSERT_NULL     1
#define LV_USE_ASSERT_MEM      1
#define LV_USE_ASSERT_MALLOC   1
#define LV_USE_ASSERT_OBJ      1
#define LV_USE_ASSERT_STYLE    1

/* Extended features */
#define LV_USE_USER_DATA       1
#define LV_USE_IMG_TRANSFORM   1

/*-------------
 * GPU settings
 *-----------*/
#define LV_USE_GPU_SDL          0
#define LV_USE_GPU_RA6M3_G2D    0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_ARM2D        0
#define LV_USE_GPU_SWM341_DMA2D 0
#define LV_USE_GPU_STM32_DMA2D  0

/*-------------
 * Fonts
 *-----------*/
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_22  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_28  1
#define LV_FONT_MONTSERRAT_32  1
#define LV_FONT_MONTSERRAT_36  1
#define LV_FONT_MONTSERRAT_38  1
#define LV_FONT_MONTSERRAT_40  1
#define LV_FONT_MONTSERRAT_42  1
#define LV_FONT_MONTSERRAT_44  1
#define LV_FONT_MONTSERRAT_46  1
#define LV_FONT_MONTSERRAT_48  1

/* Font features */
#define LV_FONT_MONTSERRAT_TEXT_UNDERLINE     1
#define LV_FONT_MONTSERRAT_TEXT_STRIKETHROUGH 1

/*-------------
 * Themes
 *-----------*/
#define LV_USE_THEME_DEFAULT  1
#define LV_USE_THEME_MONO     0
#define LV_USE_THEME_MATERIAL 1
#define LV_USE_THEME_BASIC    1

/*-------------
 * Widgets
 *-----------*/
#define LV_USE_LABEL       1
#define LV_USE_BUTTON      1
#define LV_USE_BUTTON_MATRIX 1
#define LV_USE_CANVAS      1
#define LV_USE_CHECKBOX    1
#define LV_USE_DROPDOWN    1
#define LV_USE_IMAGE       1
#define LV_USE_LINE        1
#define LV_USE_ROLLER      1
#define LV_USE_SLIDER      1
#define LV_USE_SWITCH      1
#define LV_USE_TEXTAREA    1
#define LV_USE_TABLE       1
#define LV_USE_TABVIEW     1

/*-------------
 * Additional features
 *-----------*/
#define LV_USE_ANIMATION   1
#define LV_USE_ARC         1
#define LV_USE_BAR         1
#define LV_USE_BTNMATRIX   1
#define LV_USE_CALENDAR    1
#define LV_USE_CHART       1
#define LV_USE_COLORWHEEL  1
#define LV_USE_IMGBTN      1
#define LV_USE_KEYBOARD    1
#define LV_USE_LED         1
#define LV_USE_LIST        1
#define LV_USE_MENU        1
#define LV_USE_MSGBOX      1
#define LV_USE_OBJMASK     1
#define LV_USE_SPAN        1
#define LV_USE_SPINBOX     1
#define LV_USE_SPINNER     1
#define LV_USE_TILEVIEW    1
#define LV_USE_WIN         1

/*-------------
 * File System
 *-----------*/
#define LV_USE_FS_POSIX    0
#define LV_USE_FS_FATFS    0
#define LV_USE_FS_LITTLEFS 0
#define LV_USE_FS_SPIFFS   1
#define LV_FS_SPIFFS_LETTER 'S'

/*-------------
 * Image decoders
 *-----------*/
#define LV_IMG_SUPPORTED   1
#define LV_USE_PNG         1
#define LV_USE_BMP         1
#define LV_USE_SJPG        1
#define LV_USE_GIF         1
#define LV_USE_QRCODE      1
#define LV_USE_BARCODE     1

/*-------------
 * System monitoring
 *-----------*/
#define LV_USE_SYSMON      1

#endif /*LV_CONF_H*/
