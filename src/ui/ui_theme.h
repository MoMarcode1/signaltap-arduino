// SIGNALTAP UI Theme - broncolor inspired
#ifndef UI_THEME_H
#define UI_THEME_H

#include <lvgl.h>

// ============ Color Palette ============
// Background colors
#define COLOR_BG_DARK       lv_color_hex(0x0f1115)
#define COLOR_BG_CARD       lv_color_hex(0x1f2937)
#define COLOR_BG_DARK2      lv_color_hex(0x111827)

// Border colors
#define COLOR_BORDER        lv_color_hex(0x374151)
#define COLOR_BORDER_LIGHT  lv_color_hex(0x4b5563)

// Text colors
#define COLOR_TEXT_PRIMARY  lv_color_hex(0xffffff)
#define COLOR_TEXT_MUTED    lv_color_hex(0x9ca3af)
#define COLOR_TEXT_DIM      lv_color_hex(0x6b7280)

// Accent
#define COLOR_ACCENT        lv_color_hex(0x22d3ee)

// Status colors
#define COLOR_SUCCESS       lv_color_hex(0x4ade80)
#define COLOR_WARNING       lv_color_hex(0xfacc15)
#define COLOR_ERROR         lv_color_hex(0xef4444)
#define COLOR_INFO          lv_color_hex(0x3b82f6)

// Demo profile colors
#define COLOR_CNC           lv_color_hex(0x3b82f6)
#define COLOR_CHILLER       lv_color_hex(0x06b6d4)
#define COLOR_COMPRESSOR    lv_color_hex(0x10b981)
#define COLOR_CUSTOM        lv_color_hex(0x8b5cf6)
#define COLOR_AI_PURPLE     lv_color_hex(0xa855f7)

// ============ Style Helpers ============
static inline void style_card(lv_obj_t* obj) {
    lv_obj_set_style_bg_color(obj, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 8, 0);
    lv_obj_set_style_pad_all(obj, 12, 0);
}

static inline void style_label_primary(lv_obj_t* label) {
    lv_obj_set_style_text_color(label, COLOR_TEXT_PRIMARY, 0);
}

static inline void style_label_muted(lv_obj_t* label) {
    lv_obj_set_style_text_color(label, COLOR_TEXT_MUTED, 0);
}

static inline void style_label_dim(lv_obj_t* label) {
    lv_obj_set_style_text_color(label, COLOR_TEXT_DIM, 0);
}

#endif // UI_THEME_H
