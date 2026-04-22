/*******************************************************************************
 * Size: 15 px
 * Bpp: 1
 * Opts: --bpp 1 --size 15 --font /home/nihao/SquareLine/assets/UbuntuMono-R.ttf -o /home/nihao/SquareLine/assets/ui_font_mono15.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_MONO15
#define UI_FONT_MONO15 1
#endif

#if UI_FONT_MONO15

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0x55, 0x53, 0xc0,

    /* U+0022 "\"" */
    0x99, 0x90,

    /* U+0023 "#" */
    0x14, 0x4b, 0xf9, 0x22, 0x45, 0xbf, 0xa4, 0x48,

    /* U+0024 "$" */
    0x21, 0x1f, 0x8, 0x30, 0x41, 0xf, 0x88, 0x40,

    /* U+0025 "%" */
    0xe7, 0x4a, 0xa7, 0x41, 0x5, 0xca, 0xa5, 0xce,

    /* U+0026 "&" */
    0x30, 0x91, 0x22, 0x86, 0x52, 0xa3, 0x46, 0x74,

    /* U+0027 "'" */
    0xf0,

    /* U+0028 "(" */
    0x36, 0x4c, 0x88, 0x88, 0x84, 0x63,

    /* U+0029 ")" */
    0xc6, 0x23, 0x11, 0x11, 0x12, 0x6c,

    /* U+002A "*" */
    0x25, 0x7e, 0xad, 0x0,

    /* U+002B "+" */
    0x10, 0x4f, 0xc4, 0x10, 0x40,

    /* U+002C "," */
    0x6c, 0xe0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0xf0,

    /* U+002F "/" */
    0x8, 0x46, 0x21, 0x18, 0x84, 0x62, 0x11, 0x80,

    /* U+0030 "0" */
    0x79, 0x28, 0x65, 0x96, 0x18, 0x52, 0x78,

    /* U+0031 "1" */
    0x27, 0x28, 0x42, 0x10, 0x84, 0xf8,

    /* U+0032 "2" */
    0xf2, 0x20, 0x82, 0x10, 0x84, 0x30, 0xfc,

    /* U+0033 "3" */
    0xf0, 0x20, 0x8c, 0xc, 0x10, 0x43, 0xf8,

    /* U+0034 "4" */
    0x8, 0x62, 0x9a, 0x4a, 0x2f, 0xc2, 0x8,

    /* U+0035 "5" */
    0x7d, 0x4, 0x10, 0x38, 0x10, 0x43, 0xf8,

    /* U+0036 "6" */
    0x1d, 0x84, 0x3e, 0x8e, 0x18, 0x73, 0x78,

    /* U+0037 "7" */
    0xfc, 0x30, 0x84, 0x10, 0xc2, 0x8, 0x60,

    /* U+0038 "8" */
    0x7a, 0x18, 0x73, 0x7a, 0x38, 0x61, 0x78,

    /* U+0039 "9" */
    0x7a, 0x38, 0x61, 0x7c, 0x10, 0x86, 0xe0,

    /* U+003A ":" */
    0xf0, 0x3c,

    /* U+003B ";" */
    0x6c, 0x0, 0xda, 0x80,

    /* U+003C "<" */
    0x4, 0x76, 0x30, 0x38, 0x30,

    /* U+003D "=" */
    0xfc, 0x0, 0x3f,

    /* U+003E ">" */
    0x83, 0x81, 0xc3, 0x73, 0x0,

    /* U+003F "?" */
    0xf0, 0x42, 0x22, 0x30, 0xc, 0x60,

    /* U+0040 "@" */
    0x39, 0x18, 0x67, 0xa6, 0x9a, 0x69, 0x5d, 0x83,
    0x80,

    /* U+0041 "A" */
    0x18, 0x70, 0xa1, 0x66, 0x48, 0x9f, 0xa1, 0xc2,

    /* U+0042 "B" */
    0xf2, 0x28, 0xa2, 0xf2, 0x38, 0x63, 0xf8,

    /* U+0043 "C" */
    0x3d, 0x18, 0x20, 0x82, 0x8, 0x10, 0x3c,

    /* U+0044 "D" */
    0xf2, 0x28, 0x61, 0x86, 0x18, 0x62, 0xf0,

    /* U+0045 "E" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0xf8,

    /* U+0046 "F" */
    0xfc, 0x21, 0xf, 0xc2, 0x10, 0x80,

    /* U+0047 "G" */
    0x3d, 0x18, 0x20, 0x82, 0x18, 0x51, 0x3c,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0xfe, 0x18, 0x61, 0x84,

    /* U+0049 "I" */
    0xf9, 0x8, 0x42, 0x10, 0x84, 0xf8,

    /* U+004A "J" */
    0x78, 0x42, 0x10, 0x84, 0x31, 0xf0,

    /* U+004B "K" */
    0x8a, 0x6b, 0x38, 0xe2, 0xc9, 0xa2, 0x8c,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xf8,

    /* U+004D "M" */
    0xcf, 0x3c, 0xed, 0xb6, 0xd8, 0x61, 0x84,

    /* U+004E "N" */
    0x87, 0x1e, 0x69, 0x96, 0x58, 0xe3, 0x84,

    /* U+004F "O" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xa2, 0x38,

    /* U+0050 "P" */
    0xfa, 0x38, 0x61, 0x8f, 0xe8, 0x20, 0x80,

    /* U+0051 "Q" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xe3, 0x7c,
    0x20, 0x60, 0x30,

    /* U+0052 "R" */
    0xfa, 0x38, 0x63, 0xfa, 0x68, 0xa1, 0x84,

    /* U+0053 "S" */
    0x7e, 0x8, 0x30, 0x38, 0x30, 0x41, 0xf8,

    /* U+0054 "T" */
    0xfc, 0x41, 0x4, 0x10, 0x41, 0x4, 0x10,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x73, 0x78,

    /* U+0056 "V" */
    0xc2, 0x85, 0x1a, 0x26, 0x44, 0x8a, 0x1c, 0x18,

    /* U+0057 "W" */
    0x86, 0x18, 0x6d, 0xb6, 0xdc, 0xf3, 0xcc,

    /* U+0058 "X" */
    0x46, 0xc8, 0xb0, 0xc1, 0x85, 0x9, 0x22, 0x42,

    /* U+0059 "Y" */
    0xc6, 0x89, 0x11, 0x42, 0x82, 0x4, 0x8, 0x10,

    /* U+005A "Z" */
    0xfc, 0x31, 0x84, 0x30, 0x84, 0x30, 0xfc,

    /* U+005B "[" */
    0xf2, 0x49, 0x24, 0x92, 0x70,

    /* U+005C "\\" */
    0x82, 0x10, 0xc2, 0x10, 0xc2, 0x10, 0xc2, 0x10,

    /* U+005D "]" */
    0xe4, 0x92, 0x49, 0x24, 0xf0,

    /* U+005E "^" */
    0x31, 0xc5, 0xa2, 0x84,

    /* U+005F "_" */
    0xfe,

    /* U+0060 "`" */
    0xcc,

    /* U+0061 "a" */
    0x78, 0x30, 0x5f, 0x86, 0x17, 0xc0,

    /* U+0062 "b" */
    0x82, 0x8, 0x3e, 0x8e, 0x18, 0x61, 0x8b, 0xe0,

    /* U+0063 "c" */
    0x3f, 0x8, 0x20, 0x83, 0x7, 0xc0,

    /* U+0064 "d" */
    0x4, 0x10, 0x5f, 0xc6, 0x18, 0x61, 0x45, 0xf0,

    /* U+0065 "e" */
    0x7b, 0x38, 0x7f, 0x83, 0x7, 0xc0,

    /* U+0066 "f" */
    0x1e, 0x40, 0x87, 0xe2, 0x4, 0x8, 0x10, 0x20,
    0x40,

    /* U+0067 "g" */
    0x7f, 0x18, 0x61, 0x87, 0x17, 0xc1, 0xf8,

    /* U+0068 "h" */
    0x84, 0x21, 0xe8, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0069 "i" */
    0x61, 0x80, 0x38, 0x20, 0x82, 0x8, 0x20, 0x70,

    /* U+006A "j" */
    0x18, 0xc0, 0xf0, 0x84, 0x21, 0x8, 0x43, 0xe0,

    /* U+006B "k" */
    0x82, 0x8, 0x22, 0x93, 0x8e, 0x2c, 0x9a, 0x30,

    /* U+006C "l" */
    0xe0, 0x82, 0x8, 0x20, 0x82, 0x8, 0x20, 0x70,

    /* U+006D "m" */
    0xed, 0x26, 0x4c, 0x98, 0x30, 0x60, 0x80,

    /* U+006E "n" */
    0xf4, 0x63, 0x18, 0xc6, 0x20,

    /* U+006F "o" */
    0x7b, 0x38, 0x61, 0x87, 0x37, 0x80,

    /* U+0070 "p" */
    0xf2, 0x28, 0x61, 0x86, 0x3f, 0xa0, 0x80,

    /* U+0071 "q" */
    0x3d, 0x18, 0x61, 0x87, 0x17, 0xc1, 0x4,

    /* U+0072 "r" */
    0xfc, 0x21, 0x8, 0x42, 0x0,

    /* U+0073 "s" */
    0x7c, 0x30, 0xe0, 0x87, 0xc0,

    /* U+0074 "t" */
    0x20, 0x8f, 0xc8, 0x20, 0x82, 0x8, 0x1c,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xc5, 0xe0,

    /* U+0076 "v" */
    0x42, 0x8d, 0x91, 0x22, 0x87, 0x6, 0x0,

    /* U+0077 "w" */
    0x83, 0x85, 0x6a, 0xd6, 0xac, 0x99, 0x0,

    /* U+0078 "x" */
    0x46, 0x58, 0xe0, 0xc3, 0x8c, 0x91, 0x80,

    /* U+0079 "y" */
    0x8e, 0x34, 0x92, 0x78, 0xc3, 0x8, 0xc0,

    /* U+007A "z" */
    0xf8, 0xcc, 0x44, 0x63, 0xe0,

    /* U+007B "{" */
    0x39, 0x8, 0x42, 0x60, 0x84, 0x21, 0x8, 0x30,

    /* U+007C "|" */
    0xff, 0xf0,

    /* U+007D "}" */
    0xe1, 0x8, 0x42, 0xc, 0x84, 0x21, 0x9, 0x80,

    /* U+007E "~" */
    0xe6, 0x60
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 120, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 120, .box_w = 2, .box_h = 9, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 120, .box_w = 4, .box_h = 3, .ofs_x = 2, .ofs_y = 7},
    {.bitmap_index = 6, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 14, .adv_w = 120, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 22, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 120, .box_w = 1, .box_h = 4, .ofs_x = 3, .ofs_y = 6},
    {.bitmap_index = 39, .adv_w = 120, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 45, .adv_w = 120, .box_w = 4, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 51, .adv_w = 120, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 55, .adv_w = 120, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 60, .adv_w = 120, .box_w = 3, .box_h = 4, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 62, .adv_w = 120, .box_w = 3, .box_h = 1, .ofs_x = 2, .ofs_y = 3},
    {.bitmap_index = 63, .adv_w = 120, .box_w = 2, .box_h = 2, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 64, .adv_w = 120, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 72, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 79, .adv_w = 120, .box_w = 5, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 85, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 92, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 99, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 106, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 113, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 120, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 127, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 134, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 141, .adv_w = 120, .box_w = 2, .box_h = 7, .ofs_x = 3, .ofs_y = 0},
    {.bitmap_index = 143, .adv_w = 120, .box_w = 3, .box_h = 9, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 147, .adv_w = 120, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 152, .adv_w = 120, .box_w = 6, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 155, .adv_w = 120, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 160, .adv_w = 120, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 120, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 175, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 183, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 190, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 197, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 204, .adv_w = 120, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 210, .adv_w = 120, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 216, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 223, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 230, .adv_w = 120, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 236, .adv_w = 120, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 242, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 249, .adv_w = 120, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 269, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 277, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 284, .adv_w = 120, .box_w = 7, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 295, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 302, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 316, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 323, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 331, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 346, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 354, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 361, .adv_w = 120, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 366, .adv_w = 120, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 374, .adv_w = 120, .box_w = 3, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 379, .adv_w = 120, .box_w = 6, .box_h = 5, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 383, .adv_w = 120, .box_w = 7, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 384, .adv_w = 120, .box_w = 3, .box_h = 2, .ofs_x = 2, .ofs_y = 8},
    {.bitmap_index = 385, .adv_w = 120, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 391, .adv_w = 120, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 399, .adv_w = 120, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 405, .adv_w = 120, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 413, .adv_w = 120, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 419, .adv_w = 120, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 435, .adv_w = 120, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 442, .adv_w = 120, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 450, .adv_w = 120, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 458, .adv_w = 120, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 466, .adv_w = 120, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 474, .adv_w = 120, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 481, .adv_w = 120, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 486, .adv_w = 120, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 492, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 499, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 506, .adv_w = 120, .box_w = 5, .box_h = 7, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 511, .adv_w = 120, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 516, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 523, .adv_w = 120, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 528, .adv_w = 120, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 120, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 542, .adv_w = 120, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 549, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 556, .adv_w = 120, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 561, .adv_w = 120, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 569, .adv_w = 120, .box_w = 1, .box_h = 12, .ofs_x = 3, .ofs_y = -2},
    {.bitmap_index = 571, .adv_w = 120, .box_w = 5, .box_h = 12, .ofs_x = 2, .ofs_y = -2},
    {.bitmap_index = 579, .adv_w = 120, .box_w = 6, .box_h = 2, .ofs_x = 1, .ofs_y = 3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t ui_font_mono15 = {
#else
lv_font_t ui_font_mono15 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 13,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if UI_FONT_MONO15*/

