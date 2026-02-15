/**
 * @file lv_conf.h
 * Configuration file for LVGL v9.4.0
 * SIGNALTAP - ESP32-P4 with JD9165 MIPI-DSI 1024x600
 */

#if 1 /*Set it to "1" to enable content*/

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/

#define LV_COLOR_DEPTH 16

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
    #define LV_MEM_SIZE (128 * 1024U)
    #define LV_MEM_POOL_EXPAND_SIZE 0
    #define LV_MEM_ADR 0
#endif

/*====================
   HAL SETTINGS
 *====================*/

#define LV_DEF_REFR_PERIOD  16
#define LV_DPI_DEF 130

/*=================
 * OPERATING SYSTEM
 *=================*/
#define LV_USE_OS   LV_OS_NONE

/*========================
 * RENDERING CONFIGURATION
 *========================*/

#define LV_USE_DRAW_SW 1

#if LV_USE_DRAW_SW == 1
    #define LV_DRAW_SW_SUPPORT_RGB565       1
    #define LV_DRAW_SW_SUPPORT_RGB565A8     1
    #define LV_DRAW_SW_SUPPORT_RGB888       1
    #define LV_DRAW_SW_SUPPORT_XRGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888     1
    #define LV_DRAW_SW_SUPPORT_L8           1
    #define LV_DRAW_SW_SUPPORT_AL88         1
    #define LV_DRAW_SW_SUPPORT_A8           1
    #define LV_DRAW_SW_SUPPORT_I1           1

    #define LV_DRAW_SW_COMPLEX 1

    #if LV_DRAW_SW_COMPLEX == 1
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif

    /* Enable gradient caching - required for gradient rendering */
    #define LV_GRADIENT_MAX_STOPS 2
    
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS    0
    #define LV_DRAW_SW_DRAW_UNIT_CNT            1
#endif

#define LV_USE_DRAW_VGLITE  0
#define LV_USE_DRAW_VG_LITE 0
#define LV_USE_DRAW_PXP     0
#define LV_USE_DRAW_SDL     0
#define LV_USE_DRAW_DMA2D   0
#define LV_USE_DRAW_OPENGLES 0

/* Layer buffer settings */
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE          (24 * 1024)
#define LV_DRAW_LAYER_SIMPLE_FALLBACK_BUF_SIZE 3072

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

#define LV_USE_LOG 0
#if LV_USE_LOG
    #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF 1
    #define LV_LOG_USE_TIMESTAMP 1
    #define LV_LOG_USE_FILE_LINE 1
    #define LV_LOG_TRACE_MEM        0
    #define LV_LOG_TRACE_TIMER      0
    #define LV_LOG_TRACE_INDEV      0
    #define LV_LOG_TRACE_DISP_REFR  0
    #define LV_LOG_TRACE_EVENT      0
    #define LV_LOG_TRACE_OBJ_CREATE 0
    #define LV_LOG_TRACE_LAYOUT     0
    #define LV_LOG_TRACE_ANIM       0
    #define LV_LOG_TRACE_CACHE      0
#endif

#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

#define LV_USE_REFR_DEBUG           0
#define LV_USE_LAYER_DEBUG          0
#define LV_USE_PARALLEL_DRAW_DEBUG  0

#define LV_ENABLE_GLOBAL_CUSTOM 0
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0

/*-------------
 * Compiler settings
 *-----------*/
#define LV_BIG_ENDIAN_SYSTEM 0
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 4
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_ATTRIBUTE_EXTERN_DATA

/*==================
 *   FONT USAGE
 *==================*/

#define LV_FONT_MONTSERRAT_8    0
#define LV_FONT_MONTSERRAT_10   0
#define LV_FONT_MONTSERRAT_12   0
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_18   1
#define LV_FONT_MONTSERRAT_20   0
#define LV_FONT_MONTSERRAT_22   0
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_MONTSERRAT_26   0
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_30   0
#define LV_FONT_MONTSERRAT_32   1
#define LV_FONT_MONTSERRAT_34   0
#define LV_FONT_MONTSERRAT_36   0
#define LV_FONT_MONTSERRAT_38   0
#define LV_FONT_MONTSERRAT_40   0
#define LV_FONT_MONTSERRAT_42   0
#define LV_FONT_MONTSERRAT_44   0
#define LV_FONT_MONTSERRAT_46   0
#define LV_FONT_MONTSERRAT_48   0

#define LV_FONT_MONTSERRAT_28_COMPRESSED    0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW    0
#define LV_FONT_SIMSUN_14_CJK               0
#define LV_FONT_SIMSUN_16_CJK               0
#define LV_FONT_UNSCII_8                    0
#define LV_FONT_UNSCII_16                   0

#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_FONT_FMT_TXT_LARGE   0
#define LV_USE_FONT_COMPRESSED  0
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
 *  TEXT SETTINGS
 *=================*/

#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS  " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN  0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/*==================
 * WIDGETS
 *==================*/

#define LV_WIDGETS_HAS_DEFAULT_VALUE 1

#define LV_USE_ANIMIMG      1
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR     1
#define LV_USE_CANVAS       1
#define LV_USE_CHART        1
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMAGE        1
#define LV_USE_IMAGEBUTTON  1
#define LV_USE_KEYBOARD     1
#define LV_USE_LABEL        1
#define LV_USE_LED          1
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_MENU         1
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       1
#define LV_USE_SCALE        1
#define LV_USE_SLIDER       1
#define LV_USE_SPAN         1
#define LV_USE_SPINBOX      1
#define LV_USE_SPINNER      1
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        1
#define LV_USE_TABVIEW      1
#define LV_USE_TEXTAREA     1
#define LV_USE_TILEVIEW     1
#define LV_USE_WIN          1

#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION 1
    #define LV_LABEL_LONG_TXT_HINT 1
    #define LV_LABEL_WAIT_CHAR_COUNT 3
#endif

#if LV_USE_MENU
    #define LV_MENU_BACK_BTN_VISIBILITY LV_MENU_BACK_BTN_VISIBILITY_AUTOMATIC
#endif

#if LV_USE_ROLLER
    #define LV_ROLLER_INF_PAGES 7
#endif

#if LV_USE_SPAN
    #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif

#if LV_USE_TEXTAREA
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500
#endif

/*==================
 * THEMES
 *==================*/

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK 1
    #define LV_THEME_DEFAULT_GROW 0
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO 0

/*==================
 * LAYOUTS
 *==================*/

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*==================
 * 3RD PARTY LIBRARIES
 *==================*/

#define LV_USE_FS_STDIO     0
#define LV_USE_FS_POSIX     0
#define LV_USE_FS_WIN32     0
#define LV_USE_FS_FATFS     0
#define LV_USE_FS_MEMFS     0
#define LV_USE_FS_LITTLEFS  0
#define LV_USE_FS_ARDUINO_ESP_LITTLEFS 0
#define LV_USE_FS_ARDUINO_SD 0

#define LV_USE_LODEPNG      0
#define LV_USE_LIBPNG       0
#define LV_USE_BMP          0
#define LV_USE_RLE          0
#define LV_USE_TJPGD        0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_GIF          0
#define LV_USE_QRCODE       0
#define LV_USE_BARCODE      0
#define LV_USE_FREETYPE     0
#define LV_USE_TINY_TTF     0
#define LV_USE_RLOTTIE      0
#define LV_USE_THORVG       0
#define LV_USE_LZ4          0
#define LV_USE_FFMPEG       0

/*==================
 * OTHERS
 *==================*/

#define LV_USE_SNAPSHOT         0
#define LV_USE_SYSMON           0
#define LV_USE_PROFILER         0
#define LV_USE_MONKEY           0
#define LV_USE_GRIDNAV          0
#define LV_USE_FRAGMENT         0
#define LV_USE_IMGFONT          0
#define LV_USE_OBSERVER         1
#define LV_USE_IME_PINYIN       0
#define LV_USE_FILE_EXPLORER    0
#define LV_USE_XML              0

/*==================
 * DEMOS
 *==================*/

#define LV_USE_DEMO_WIDGETS         0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER 0
#define LV_USE_DEMO_BENCHMARK       0
#define LV_USE_DEMO_STRESS          0
#define LV_USE_DEMO_MUSIC           0
#define LV_USE_DEMO_FLEX_LAYOUT     0
#define LV_USE_DEMO_MULTILANG       0
#define LV_USE_DEMO_TRANSFORM       0
#define LV_USE_DEMO_SCROLL          0
#define LV_USE_DEMO_VECTOR_GRAPHIC  0
#define LV_USE_DEMO_EBIKE           0

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/
