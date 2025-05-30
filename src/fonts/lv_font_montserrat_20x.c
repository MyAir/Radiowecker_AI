/*******************************************************************************
 * Size: 20 px
 * Bpp: 1
 * Opts: --bpp 1 --size 20 --no-compress --font Montserrat-Medium.ttf --symbols ÄÖÜäöü⛅☔ --format lvgl -o lv_font_montserrat_20x.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LV_FONT_MONTSERRAT_20X
#define LV_FONT_MONTSERRAT_20X 1
#endif

#if LV_FONT_MONTSERRAT_20X

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+00C4 "Ä" */
    0xc, 0xc0, 0x0, 0x0, 0x0, 0x0, 0xc0, 0x7,
    0x80, 0x1e, 0x0, 0xcc, 0x3, 0x30, 0x18, 0x60,
    0x61, 0x83, 0x87, 0xc, 0xc, 0x3f, 0xf1, 0x80,
    0x66, 0x1, 0xb0, 0x3, 0xc0, 0xc,

    /* U+00D6 "Ö" */
    0x6, 0xc0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x7c, 0x3, 0x6, 0xc, 0x6, 0x30, 0x6, 0xc0,
    0x7, 0x80, 0xf, 0x0, 0x1e, 0x0, 0x3c, 0x0,
    0x78, 0x0, 0xd8, 0x3, 0x18, 0xc, 0x18, 0x30,
    0xf, 0x80,

    /* U+00DC "Ü" */
    0x19, 0x80, 0x0, 0x0, 0x0, 0x0, 0xc0, 0x3c,
    0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x3c, 0x3, 0xc0,
    0x3c, 0x3, 0xc0, 0x3c, 0x3, 0xc0, 0x36, 0x6,
    0x30, 0xc1, 0xf8,

    /* U+00E4 "ä" */
    0x33, 0x0, 0x0, 0x0, 0x7, 0xe2, 0x18, 0x6,
    0x3, 0x1, 0xbf, 0xf8, 0x78, 0x3c, 0x1f, 0x1c,
    0xfe,

    /* U+00F6 "ö" */
    0x1b, 0x0, 0x0, 0x0, 0x0, 0x1, 0xf0, 0x63,
    0x18, 0x36, 0x3, 0xc0, 0x78, 0xf, 0x1, 0xe0,
    0x36, 0xc, 0x63, 0x7, 0xc0,

    /* U+00FC "ü" */
    0x33, 0x0, 0x0, 0x0, 0x0, 0xc0, 0xf0, 0x3c,
    0xf, 0x3, 0xc0, 0xf0, 0x3c, 0xf, 0x3, 0xc1,
    0xd8, 0x73, 0xec
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 234, .box_w = 14, .box_h = 17, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 269, .box_w = 15, .box_h = 18, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 253, .box_w = 12, .box_h = 18, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 91, .adv_w = 191, .box_w = 9, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 108, .adv_w = 203, .box_w = 11, .box_h = 15, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 129, .adv_w = 217, .box_w = 10, .box_h = 15, .ofs_x = 2, .ofs_y = 0}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_0[] = {
    0x0, 0x12, 0x18, 0x20, 0x32, 0x38
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 196, .range_length = 57, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 6, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    1, 1,
    1, 2,
    1, 3,
    1, 5,
    1, 6,
    2, 1,
    3, 1,
    4, 5,
    5, 1,
    5, 3,
    5, 4,
    6, 3
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    4, -3, -5, -2, -3, -3, -5, 1,
    -4, -6, -2, -4
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 12,
    .glyph_ids_size = 0
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_pairs,
    .kern_scale = 16,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};

extern const lv_font_t lv_font_montserrat_20;


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t lv_font_montserrat_20x = {
#else
lv_font_t lv_font_montserrat_20x = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 18,          /*The maximum line height required by the font*/
    .base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
#if LV_VERSION_CHECK(8, 2, 0) || LVGL_VERSION_MAJOR >= 9
    .fallback = &lv_font_montserrat_20,
#endif
    .user_data = NULL,
};



#endif /*#if LV_FONT_MONTSERRAT_20X*/

