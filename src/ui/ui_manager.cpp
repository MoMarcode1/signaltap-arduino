// SIGNALTAP UI Manager Implementation - Full Featured
#include "ui_manager.h"
#include "ui_theme.h"
#include "../../config.h"
#include "../data/simulation_engine.h"
#include <stdio.h>
#include <string.h>

// External logo
LV_IMG_DECLARE(Gemini_Generated_Image_byf1vbyf1vbyf1jvb);

// ============ Static Variables ============
static UIState_t uiState = {
    .currentScreen = SCREEN_SPLASH,
    .sidebarCollapsed = false,
    .systemRunning = true,
    .setupCompleted = false,
    .unackedAlarms = 2
};

static lv_obj_t* screens[SCREEN_COUNT] = {NULL};
static lv_obj_t* sidebar = NULL;
static lv_obj_t* header = NULL;
static lv_obj_t* contentArea = NULL;
static lv_obj_t* mainContainer = NULL;

// Nav button references
#define NAV_BUTTON_COUNT 7
static lv_obj_t* navButtons[NAV_BUTTON_COUNT] = {NULL};

// Header elements for updating
static lv_obj_t* demoNameLabel = NULL;
static lv_obj_t* demoIndicator = NULL;
static lv_obj_t* statusBadge = NULL;
static lv_obj_t* statusLabel = NULL;
static lv_obj_t* powerBtn = NULL;
static lv_obj_t* scenarioBadge = NULL;
static lv_obj_t* scenarioLabel = NULL;

// Screen content containers (for refreshing)
static lv_obj_t* setupContent = NULL;
static lv_obj_t* homeContent = NULL;
static lv_obj_t* sensorsContent = NULL;
static lv_obj_t* alarmsContent = NULL;
static lv_obj_t* visionContent = NULL;
static lv_obj_t* aiContent = NULL;
static lv_obj_t* remoteContent = NULL;

// Device ID for QR code
static char deviceId[32] = "STAP-001-A7F3";

// ============ Forward Declarations ============
static void create_sidebar(lv_obj_t* parent);
static void create_header(lv_obj_t* parent);
static void create_content_area(lv_obj_t* parent);
static void update_nav_highlight(void);

static void create_splash_screen(void);
static void create_setup_screen(void);
static void create_home_screen(void);
static void create_sensors_screen(void);
static void create_alarms_screen(void);
static void create_vision_screen(void);
static void create_ai_screen(void);
static void create_remote_screen(void);
static void create_settings_screen(void);

static void rebuild_setup_content(void);
static void rebuild_home_content(void);
static void rebuild_sensors_content(void);
static void rebuild_alarms_content(void);
static void rebuild_vision_content(void);
static void rebuild_ai_content(void);
static void rebuild_remote_content(void);
static void update_header_demo(void);
static void create_vision_panel_content(lv_obj_t* parent, DemoProfile_t* demo, int yOffset);
static void create_qr_code(lv_obj_t* parent, const char* data, int size);
static void create_insight_card(lv_obj_t* parent, AIInsight_t* insight, int width);

// ============ Event Handlers ============
static void nav_btn_event_cb(lv_event_t* e) {
    intptr_t screenId = (intptr_t)lv_event_get_user_data(e);
    ui_navigate_to((ScreenID_t)screenId);
}

static void demo_btn_event_cb(lv_event_t* e) {
    nextDemo();
    update_header_demo();
    ui_refresh();
}

static void power_btn_event_cb(lv_event_t* e) {
    ui_toggle_system();
}

static void ack_btn_event_cb(lv_event_t* e) {
    intptr_t alarmIdx = (intptr_t)lv_event_get_user_data(e);
    sim_ack_alarm((uint8_t)alarmIdx);
    rebuild_alarms_content();
    rebuild_home_content();
}

static void ota_btn_event_cb(lv_event_t* e) {
    (void)e;
    if (!sim_ota_active()) {
        sim_start_ota();
        rebuild_ai_content();
    }
}

static void setup_next_demo_event_cb(lv_event_t* e) {
    (void)e;
    nextDemo();
    update_header_demo();
    rebuild_setup_content();
    ui_refresh();
}

static void setup_finish_event_cb(lv_event_t* e) {
    (void)e;
    uiState.setupCompleted = true;
    ui_navigate_to(SCREEN_HOME);
}

// ============ Public Functions ============
void ui_init(void) {
    create_splash_screen();
    
    mainContainer = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(mainContainer, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(mainContainer, LV_OPA_COVER, 0);
    lv_obj_set_size(mainContainer, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_clear_flag(mainContainer, LV_OBJ_FLAG_SCROLLABLE);
    
    create_sidebar(mainContainer);
    
    lv_obj_t* rightSide = lv_obj_create(mainContainer);
    lv_obj_set_style_bg_opa(rightSide, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rightSide, 0, 0);
    lv_obj_set_style_pad_all(rightSide, 0, 0);
    lv_obj_set_size(rightSide, DISPLAY_WIDTH - SIDEBAR_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(rightSide, SIDEBAR_WIDTH, 0);
    lv_obj_clear_flag(rightSide, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(rightSide, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(rightSide, LV_FLEX_FLOW_COLUMN);
    
    create_header(rightSide);
    create_content_area(rightSide);
    
    create_setup_screen();
    create_home_screen();
    create_sensors_screen();
    create_alarms_screen();
    create_vision_screen();
    create_ai_screen();
    create_remote_screen();
    create_settings_screen();
    
    lv_scr_load(screens[SCREEN_SPLASH]);
}

void ui_show_main(void) {
    lv_scr_load(mainContainer);
#if ENABLE_ONBOARDING
    if (!uiState.setupCompleted) {
        ui_navigate_to(SCREEN_SETUP);
        return;
    }
#endif
    ui_navigate_to(SCREEN_HOME);
}

void ui_navigate_to(ScreenID_t screen) {
    if (screen >= SCREEN_COUNT || screen == SCREEN_SPLASH) return;
    
    uiState.currentScreen = screen;
    update_nav_highlight();
    
    for (int i = SCREEN_SETUP; i < SCREEN_COUNT; i++) {
        if (screens[i]) {
            if (i == screen) {
                lv_obj_clear_flag(screens[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_add_flag(screens[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void ui_refresh(void) {
    update_scenario_badge();
    rebuild_setup_content();
    rebuild_home_content();
    rebuild_sensors_content();
    rebuild_alarms_content();
    rebuild_vision_content();
    rebuild_ai_content();
    rebuild_remote_content();
    // Settings screen shows live sim state
    if (uiState.currentScreen == SCREEN_SETTINGS) {
        rebuild_settings_content();
    }
}

UIState_t* ui_get_state(void) {
    return &uiState;
}

void ui_toggle_sidebar(void) {
    uiState.sidebarCollapsed = !uiState.sidebarCollapsed;
    lv_obj_set_width(sidebar, uiState.sidebarCollapsed ? SIDEBAR_COLLAPSED : SIDEBAR_WIDTH);
}

void ui_toggle_system(void) {
    uiState.systemRunning = !uiState.systemRunning;
    
    // Update status badge
    if (statusBadge && statusLabel) {
        if (uiState.systemRunning) {
            lv_obj_set_style_bg_color(statusBadge, lv_color_hex(0x14532d), 0);
            lv_label_set_text(statusLabel, "RUNNING");
            lv_obj_set_style_text_color(statusLabel, COLOR_SUCCESS, 0);
        } else {
            lv_obj_set_style_bg_color(statusBadge, lv_color_hex(0x422006), 0);
            lv_label_set_text(statusLabel, "STOPPED");
            lv_obj_set_style_text_color(statusLabel, COLOR_WARNING, 0);
        }
    }
}

void ui_update_sensors(void) {
    if (!uiState.systemRunning) return;
    rebuild_home_content();
    rebuild_sensors_content();
}

// ============ Splash Screen ============
static void create_splash_screen(void) {
    screens[SCREEN_SPLASH] = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screens[SCREEN_SPLASH], lv_color_hex(0x3034be), 0);  // Blue background like logo
    lv_obj_set_style_bg_opa(screens[SCREEN_SPLASH], LV_OPA_COVER, 0);
    lv_obj_set_size(screens[SCREEN_SPLASH], DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_clear_flag(screens[SCREEN_SPLASH], LV_OBJ_FLAG_SCROLLABLE);
    
    // Logo image
    lv_obj_t* logo = lv_image_create(screens[SCREEN_SPLASH]);
    lv_image_set_src(logo, &Gemini_Generated_Image_byf1vbyf1vbyf1jvb);
    lv_obj_center(logo);
    
    // Loading spinner below logo
    lv_obj_t* spinner = lv_spinner_create(screens[SCREEN_SPLASH]);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, 140);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0xffffff), LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(0x5058d0), LV_PART_MAIN);
}

// ============ Setup Screen ============
static void create_setup_screen(void) {
    screens[SCREEN_SETUP] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_SETUP], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_SETUP], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_SETUP], 0, 0);
    lv_obj_set_size(screens[SCREEN_SETUP], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_SETUP], 0, 0);
    lv_obj_add_flag(screens[SCREEN_SETUP], LV_OBJ_FLAG_HIDDEN);

    setupContent = lv_obj_create(screens[SCREEN_SETUP]);
    lv_obj_set_style_bg_opa(setupContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(setupContent, 0, 0);
    lv_obj_set_style_pad_all(setupContent, 0, 0);
    lv_obj_set_size(setupContent, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(setupContent, LV_OBJ_FLAG_SCROLLABLE);

    rebuild_setup_content();
}

static void rebuild_setup_content(void) {
    if (!setupContent) return;
    lv_obj_clean(setupContent);

    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;
    DemoProfile_t* demo = getDemo();

    lv_obj_t* title = lv_label_create(setupContent);
    lv_label_set_text(title, "Welcome to SIGNALTAP");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(title, 0, 0);

    lv_obj_t* subtitle = lv_label_create(setupContent);
    lv_label_set_text(subtitle, "Quick setup for first-time users (demo mode)");
    lv_obj_set_style_text_color(subtitle, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(subtitle, 0, 30);

    lv_obj_t* setupCard = lv_obj_create(setupContent);
    style_card(setupCard);
    lv_obj_set_size(setupCard, contentWidth, 270);
    lv_obj_set_pos(setupCard, 0, 60);
    lv_obj_clear_flag(setupCard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* step1 = lv_label_create(setupCard);
    lv_label_set_text(step1, "1) Choose a demo profile");
    lv_obj_set_style_text_color(step1, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_pos(step1, 0, 0);

    lv_obj_t* demoBadge = lv_obj_create(setupCard);
    lv_obj_set_size(demoBadge, 360, 54);
    lv_obj_set_pos(demoBadge, 0, 25);
    lv_obj_set_style_bg_color(demoBadge, COLOR_BG_DARK2, 0);
    lv_obj_set_style_bg_opa(demoBadge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(demoBadge, 1, 0);
    lv_obj_set_style_border_color(demoBadge, lv_color_hex(demo->color), 0);
    lv_obj_set_style_radius(demoBadge, 6, 0);
    lv_obj_clear_flag(demoBadge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* demoName = lv_label_create(demoBadge);
    lv_label_set_text(demoName, demo->name);
    lv_obj_set_style_text_color(demoName, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(demoName, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(demoName, 10, 7);

    lv_obj_t* demoSub = lv_label_create(demoBadge);
    lv_label_set_text(demoSub, demo->sub);
    lv_obj_set_style_text_color(demoSub, COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(demoSub, 10, 30);

    lv_obj_t* nextDemoBtn = lv_btn_create(setupCard);
    lv_obj_set_size(nextDemoBtn, 190, 34);
    lv_obj_set_pos(nextDemoBtn, 370, 35);
    lv_obj_set_style_bg_color(nextDemoBtn, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(nextDemoBtn, 6, 0);
    lv_obj_set_style_shadow_width(nextDemoBtn, 0, 0);
    lv_obj_add_event_cb(nextDemoBtn, setup_next_demo_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* nextDemoLabel = lv_label_create(nextDemoBtn);
    lv_label_set_text(nextDemoLabel, "Switch Demo");
    lv_obj_set_style_text_color(nextDemoLabel, COLOR_BG_DARK, 0);
    lv_obj_center(nextDemoLabel);

    lv_obj_t* step2 = lv_label_create(setupCard);
    lv_label_set_text(step2, "2) Remote dashboard URL (QR target)");
    lv_obj_set_style_text_color(step2, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_pos(step2, 0, 95);

    lv_obj_t* urlBox = lv_obj_create(setupCard);
    lv_obj_set_size(urlBox, contentWidth - 24, 62);
    lv_obj_set_pos(urlBox, 0, 120);
    lv_obj_set_style_bg_color(urlBox, COLOR_BG_DARK2, 0);
    lv_obj_set_style_bg_opa(urlBox, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(urlBox, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(urlBox, 1, 0);
    lv_obj_set_style_radius(urlBox, 6, 0);
    lv_obj_clear_flag(urlBox, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* urlLabel = lv_label_create(urlBox);
    lv_label_set_text_fmt(urlLabel, "%s?device=%s", REMOTE_DASHBOARD_URL, deviceId);
    lv_obj_set_style_text_color(urlLabel, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(urlLabel, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(urlLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(urlLabel, contentWidth - 48);
    lv_obj_set_pos(urlLabel, 8, 8);

    lv_obj_t* finishBtn = lv_btn_create(setupCard);
    lv_obj_set_size(finishBtn, 180, 38);
    lv_obj_set_pos(finishBtn, contentWidth - 214, 214);
    lv_obj_set_style_bg_color(finishBtn, COLOR_SUCCESS, 0);
    lv_obj_set_style_radius(finishBtn, 6, 0);
    lv_obj_set_style_shadow_width(finishBtn, 0, 0);
    lv_obj_add_event_cb(finishBtn, setup_finish_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* finishLabel = lv_label_create(finishBtn);
    lv_label_set_text(finishLabel, "Finish Setup");
    lv_obj_set_style_text_color(finishLabel, COLOR_BG_DARK, 0);
    lv_obj_center(finishLabel);
}

// ============ Sidebar ============
static void create_sidebar(lv_obj_t* parent) {
    sidebar = lv_obj_create(parent);
    lv_obj_set_style_bg_color(sidebar, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(sidebar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sidebar, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(sidebar, 1, 0);
    lv_obj_set_style_border_side(sidebar, LV_BORDER_SIDE_RIGHT, 0);
    lv_obj_set_style_radius(sidebar, 0, 0);
    lv_obj_set_style_pad_all(sidebar, 8, 0);
    lv_obj_set_size(sidebar, SIDEBAR_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(sidebar, 0, 0);
    lv_obj_clear_flag(sidebar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(sidebar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sidebar, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(sidebar, 4, 0);
    
    // Logo header
    lv_obj_t* logoContainer = lv_obj_create(sidebar);
    lv_obj_set_style_bg_opa(logoContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(logoContainer, 0, 0);
    lv_obj_set_style_pad_all(logoContainer, 4, 0);
    lv_obj_set_size(logoContainer, SIDEBAR_WIDTH - 20, 36);
    lv_obj_clear_flag(logoContainer, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* logoLabel = lv_label_create(logoContainer);
    lv_label_set_text(logoLabel, LV_SYMBOL_SETTINGS " SIGNALTAP");
    lv_obj_set_style_text_color(logoLabel, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(logoLabel, &lv_font_montserrat_14, 0);
    lv_obj_center(logoLabel);
    
    // Separator
    lv_obj_t* sep = lv_obj_create(sidebar);
    lv_obj_set_size(sep, SIDEBAR_WIDTH - 20, 1);
    lv_obj_set_style_bg_color(sep, COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_set_style_radius(sep, 0, 0);
    lv_obj_set_style_pad_all(sep, 0, 0);
    
    // Navigation buttons
    const char* navLabels[] = {
        LV_SYMBOL_HOME " Home",
        LV_SYMBOL_EYE_OPEN " Sensors",
        LV_SYMBOL_WARNING " Alarms",
        LV_SYMBOL_IMAGE " Vision",
        LV_SYMBOL_WIFI " AI Agent",
        LV_SYMBOL_DOWNLOAD " Remote View",
        LV_SYMBOL_SETTINGS " Settings"
    };
    const ScreenID_t navScreens[] = {
        SCREEN_HOME,
        SCREEN_SENSORS,
        SCREEN_ALARMS,
        SCREEN_VISION,
        SCREEN_AI,
        SCREEN_REMOTE,
        SCREEN_SETTINGS
    };
    
    for (int i = 0; i < NAV_BUTTON_COUNT; i++) {
        lv_obj_t* btn = lv_btn_create(sidebar);
        lv_obj_set_size(btn, SIDEBAR_WIDTH - 20, 40);
        lv_obj_set_style_bg_color(btn, COLOR_BG_DARK2, 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_style_bg_color(btn, COLOR_BORDER, LV_STATE_PRESSED);
        
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, navLabels[i]);
        lv_obj_set_style_text_color(label, COLOR_TEXT_MUTED, 0);
        lv_obj_align(label, LV_ALIGN_LEFT_MID, 8, 0);
        
        lv_obj_add_event_cb(btn, nav_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)navScreens[i]);
        navButtons[i] = btn;
    }
}

static void update_nav_highlight(void) {
    const ScreenID_t navScreens[] = {
        SCREEN_HOME,
        SCREEN_SENSORS,
        SCREEN_ALARMS,
        SCREEN_VISION,
        SCREEN_AI,
        SCREEN_REMOTE,
        SCREEN_SETTINGS
    };
    for (int i = 0; i < NAV_BUTTON_COUNT; i++) {
        if (navButtons[i]) {
            lv_obj_t* label = lv_obj_get_child(navButtons[i], 0);
            if (navScreens[i] == uiState.currentScreen) {
                lv_obj_set_style_bg_color(navButtons[i], lv_color_hex(0x22d3ee20), 0);
                lv_obj_set_style_border_width(navButtons[i], 2, 0);
                lv_obj_set_style_border_color(navButtons[i], COLOR_ACCENT, 0);
                lv_obj_set_style_border_side(navButtons[i], LV_BORDER_SIDE_LEFT, 0);
                if (label) lv_obj_set_style_text_color(label, COLOR_ACCENT, 0);
            } else {
                lv_obj_set_style_bg_color(navButtons[i], COLOR_BG_DARK2, 0);
                lv_obj_set_style_border_width(navButtons[i], 0, 0);
                if (label) lv_obj_set_style_text_color(label, COLOR_TEXT_MUTED, 0);
            }
        }
    }
}

// ============ Header ============
static void update_header_demo(void) {
    DemoProfile_t* demo = getDemo();
    if (demoNameLabel) {
        lv_label_set_text(demoNameLabel, demo->name);
    }
    if (demoIndicator) {
        lv_obj_set_style_bg_color(demoIndicator, lv_color_hex(demo->color), 0);
    }
}

static void create_header(lv_obj_t* parent) {
    header = lv_obj_create(parent);
    lv_obj_set_style_bg_color(header, COLOR_BG_CARD, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(header, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 12, 0);
    lv_obj_set_style_pad_ver(header, 6, 0);
    lv_obj_set_size(header, DISPLAY_WIDTH - SIDEBAR_WIDTH, HEADER_HEIGHT);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_grow(header, 0);
    
    // Demo selector button
    lv_obj_t* demoBtn = lv_btn_create(header);
    lv_obj_set_size(demoBtn, 220, 32);
    lv_obj_set_style_bg_color(demoBtn, COLOR_BG_DARK2, 0);
    lv_obj_set_style_border_color(demoBtn, COLOR_BORDER_LIGHT, 0);
    lv_obj_set_style_border_width(demoBtn, 1, 0);
    lv_obj_set_style_radius(demoBtn, 6, 0);
    lv_obj_set_style_shadow_width(demoBtn, 0, 0);
    lv_obj_set_pos(demoBtn, 0, 4);
    
    // Demo color indicator
    demoIndicator = lv_obj_create(demoBtn);
    lv_obj_set_size(demoIndicator, 16, 16);
    lv_obj_set_style_radius(demoIndicator, 4, 0);
    lv_obj_set_style_bg_color(demoIndicator, lv_color_hex(getDemo()->color), 0);
    lv_obj_set_style_bg_opa(demoIndicator, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(demoIndicator, 0, 0);
    lv_obj_align(demoIndicator, LV_ALIGN_LEFT_MID, 6, 0);
    
    demoNameLabel = lv_label_create(demoBtn);
    lv_label_set_text(demoNameLabel, getDemo()->name);
    lv_obj_set_style_text_color(demoNameLabel, COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(demoNameLabel, LV_ALIGN_LEFT_MID, 28, 0);
    
    lv_obj_t* dropdownIcon = lv_label_create(demoBtn);
    lv_label_set_text(dropdownIcon, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(dropdownIcon, COLOR_TEXT_MUTED, 0);
    lv_obj_align(dropdownIcon, LV_ALIGN_RIGHT_MID, -6, 0);
    
    lv_obj_add_event_cb(demoBtn, demo_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Scenario state badge
    scenarioBadge = lv_obj_create(header);
    lv_obj_set_size(scenarioBadge, 90, 26);
    lv_obj_set_style_bg_color(scenarioBadge, lv_color_hex(0x14532d), 0);
    lv_obj_set_style_bg_opa(scenarioBadge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(scenarioBadge, 4, 0);
    lv_obj_set_style_border_width(scenarioBadge, 0, 0);
    lv_obj_set_pos(scenarioBadge, DISPLAY_WIDTH - SIDEBAR_WIDTH - 310, 8);
    lv_obj_clear_flag(scenarioBadge, LV_OBJ_FLAG_SCROLLABLE);

    scenarioLabel = lv_label_create(scenarioBadge);
    lv_label_set_text(scenarioLabel, "Normal");
    lv_obj_set_style_text_color(scenarioLabel, COLOR_SUCCESS, 0);
    lv_obj_center(scenarioLabel);

    // Status badge
    statusBadge = lv_obj_create(header);
    lv_obj_set_size(statusBadge, 85, 26);
    lv_obj_set_style_bg_color(statusBadge, lv_color_hex(0x14532d), 0);
    lv_obj_set_style_bg_opa(statusBadge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(statusBadge, 13, 0);
    lv_obj_set_style_border_width(statusBadge, 0, 0);
    lv_obj_set_pos(statusBadge, DISPLAY_WIDTH - SIDEBAR_WIDTH - 180, 7);
    lv_obj_clear_flag(statusBadge, LV_OBJ_FLAG_SCROLLABLE);
    
    // Status dot
    lv_obj_t* statusDot = lv_obj_create(statusBadge);
    lv_obj_set_size(statusDot, 6, 6);
    lv_obj_set_style_radius(statusDot, 3, 0);
    lv_obj_set_style_bg_color(statusDot, COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(statusDot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(statusDot, 0, 0);
    lv_obj_align(statusDot, LV_ALIGN_LEFT_MID, 8, 0);
    
    statusLabel = lv_label_create(statusBadge);
    lv_label_set_text(statusLabel, "RUNNING");
    lv_obj_set_style_text_color(statusLabel, COLOR_SUCCESS, 0);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(statusLabel, LV_ALIGN_LEFT_MID, 18, 0);
    
    // Power button
    powerBtn = lv_btn_create(header);
    lv_obj_set_size(powerBtn, 32, 32);
    lv_obj_set_style_bg_color(powerBtn, lv_color_hex(0x3f1010), 0);
    lv_obj_set_style_radius(powerBtn, 6, 0);
    lv_obj_set_style_shadow_width(powerBtn, 0, 0);
    lv_obj_set_pos(powerBtn, DISPLAY_WIDTH - SIDEBAR_WIDTH - 80, 4);
    
    lv_obj_t* powerIcon = lv_label_create(powerBtn);
    lv_label_set_text(powerIcon, LV_SYMBOL_POWER);
    lv_obj_set_style_text_color(powerIcon, COLOR_ERROR, 0);
    lv_obj_center(powerIcon);
    
    lv_obj_add_event_cb(powerBtn, power_btn_event_cb, LV_EVENT_CLICKED, NULL);
}

// ============ Content Area ============
static void create_content_area(lv_obj_t* parent) {
    contentArea = lv_obj_create(parent);
    lv_obj_set_style_bg_color(contentArea, COLOR_BG_DARK, 0);
    lv_obj_set_style_bg_opa(contentArea, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(contentArea, 0, 0);
    lv_obj_set_style_pad_all(contentArea, 12, 0);
    lv_obj_set_size(contentArea, DISPLAY_WIDTH - SIDEBAR_WIDTH, DISPLAY_HEIGHT - HEADER_HEIGHT);
    lv_obj_set_flex_grow(contentArea, 1);
    lv_obj_clear_flag(contentArea, LV_OBJ_FLAG_SCROLLABLE);
}

// ============ Helper: Create Sensor Card ============
static lv_obj_t* create_sensor_card(lv_obj_t* parent, Sensor_t* sensor, int width) {
    lv_obj_t* card = lv_obj_create(parent);
    style_card(card);
    lv_obj_set_size(card, width, 95);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Status dot
    lv_obj_t* statusDot = lv_obj_create(card);
    lv_obj_set_size(statusDot, 6, 6);
    lv_obj_set_style_radius(statusDot, 3, 0);
    lv_obj_set_style_bg_color(statusDot, COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(statusDot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(statusDot, 0, 0);
    lv_obj_align(statusDot, LV_ALIGN_TOP_RIGHT, 0, 0);
    
    // Name
    lv_obj_t* nameLabel = lv_label_create(card);
    lv_label_set_text(nameLabel, sensor->name);
    lv_obj_set_style_text_color(nameLabel, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_font(nameLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(nameLabel, 0, 0);
    
    // Type
    lv_obj_t* typeLabel = lv_label_create(card);
    lv_label_set_text(typeLabel, sensor->type);
    lv_obj_set_style_text_color(typeLabel, COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(typeLabel, 0, 16);
    
    // Value
    char valStr[32];
    snprintf(valStr, sizeof(valStr), "%.*f", sensor->decimals, sensor->value);
    lv_obj_t* valLabel = lv_label_create(card);
    lv_label_set_text(valLabel, valStr);
    lv_obj_set_style_text_color(valLabel, lv_color_hex(sensor->color), 0);
    lv_obj_set_style_text_font(valLabel, &lv_font_montserrat_24, 0);
    lv_obj_set_pos(valLabel, 0, 30);
    
    // Unit
    lv_obj_t* unitLabel = lv_label_create(card);
    lv_label_set_text(unitLabel, sensor->unit);
    lv_obj_set_style_text_color(unitLabel, COLOR_TEXT_MUTED, 0);
    lv_obj_align_to(unitLabel, valLabel, LV_ALIGN_OUT_RIGHT_BOTTOM, 4, 0);
    
    // Progress bar
    lv_obj_t* bar = lv_bar_create(card);
    lv_obj_set_size(bar, width - 24, 6);
    lv_obj_set_pos(bar, 0, 70);
    lv_obj_set_style_bg_color(bar, COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(sensor->color), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 3, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 3, LV_PART_INDICATOR);
    
    int pct = (int)(((sensor->value - sensor->min) / (sensor->max - sensor->min)) * 100);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    lv_bar_set_value(bar, pct, LV_ANIM_OFF);
    
    // Range labels
    char minStr[16], maxStr[16];
    snprintf(minStr, sizeof(minStr), "%.0f", sensor->min);
    snprintf(maxStr, sizeof(maxStr), "%.0f", sensor->max);
    
    lv_obj_t* minLabel = lv_label_create(card);
    lv_label_set_text(minLabel, minStr);
    lv_obj_set_style_text_color(minLabel, COLOR_TEXT_DIM, 0);
    lv_obj_set_pos(minLabel, 0, 78);
    
    lv_obj_t* maxLabel = lv_label_create(card);
    lv_label_set_text(maxLabel, maxStr);
    lv_obj_set_style_text_color(maxLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(maxLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    
    return card;
}

// ============ Helper: Create KPI Card ============
static lv_obj_t* create_kpi_card(lv_obj_t* parent, KPI_t* kpi, int width) {
    lv_obj_t* card = lv_obj_create(parent);
    style_card(card);
    lv_obj_set_size(card, width, 70);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* labelObj = lv_label_create(card);
    lv_label_set_text(labelObj, kpi->label);
    lv_obj_set_style_text_color(labelObj, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(labelObj, 0, 0);
    
    char valStr[32];
    snprintf(valStr, sizeof(valStr), "%s%s", kpi->value, kpi->unit);
    lv_obj_t* valLabel = lv_label_create(card);
    lv_label_set_text(valLabel, valStr);
    lv_obj_set_style_text_color(valLabel, kpi->good ? COLOR_SUCCESS : COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(valLabel, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(valLabel, 0, 22);
    
    return card;
}

// ============ Helper: Update Scenario Badge ============
static void update_scenario_badge(void) {
    if (!scenarioBadge || !scenarioLabel) return;
    const char* name = sim_get_scenario_name();
    ScenarioState_t state = sim_get_scenario();

    lv_label_set_text(scenarioLabel, name);

    lv_color_t bgColor, textColor;
    switch (state) {
        case SCENARIO_NORMAL:
            bgColor = lv_color_hex(0x14532d); textColor = COLOR_SUCCESS; break;
        case SCENARIO_DEGRADATION:
            bgColor = lv_color_hex(0x422006); textColor = COLOR_WARNING; break;
        case SCENARIO_WARNING:
            bgColor = lv_color_hex(0x78350f); textColor = lv_color_hex(0xfbbf24); break;
        case SCENARIO_FAULT:
            bgColor = lv_color_hex(0x7f1d1d); textColor = COLOR_ERROR; break;
        case SCENARIO_RECOVERY:
            bgColor = lv_color_hex(0x1e3a5f); textColor = COLOR_INFO; break;
        default:
            bgColor = lv_color_hex(0x14532d); textColor = COLOR_SUCCESS; break;
    }
    lv_obj_set_style_bg_color(scenarioBadge, bgColor, 0);
    lv_obj_set_style_text_color(scenarioLabel, textColor, 0);
}

// ============ Helper: Create Dynamic Alarm Row ============
static lv_obj_t* create_dynamic_alarm_row(lv_obj_t* parent, DynamicAlarm_t* alarm, int index, bool showAckBtn) {
    if (!alarm || !alarm->active || strlen(alarm->message) == 0) return NULL;

    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(row, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_set_size(row, lv_pct(100), 40);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    if (alarm->acked) {
        lv_obj_set_style_opa(row, LV_OPA_50, 0);
    }

    // Severity dot
    lv_obj_t* dot = lv_obj_create(row);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, 4, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);

    if (strcmp(alarm->severity, "error") == 0) {
        lv_obj_set_style_bg_color(dot, COLOR_ERROR, 0);
    } else if (strcmp(alarm->severity, "warning") == 0) {
        lv_obj_set_style_bg_color(dot, COLOR_WARNING, 0);
    } else {
        lv_obj_set_style_bg_color(dot, COLOR_INFO, 0);
    }
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);

    // Message
    lv_obj_t* msgLabel = lv_label_create(row);
    lv_label_set_text(msgLabel, alarm->message);
    lv_obj_set_style_text_color(msgLabel, alarm->acked ? COLOR_TEXT_DIM : COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(msgLabel, &lv_font_montserrat_12, 0);
    lv_obj_align(msgLabel, LV_ALIGN_LEFT_MID, 16, 0);
    lv_obj_set_width(msgLabel, showAckBtn ? 350 : 420);
    lv_label_set_long_mode(msgLabel, LV_LABEL_LONG_DOT);

    // Time
    lv_obj_t* timeLabel = lv_label_create(row);
    lv_label_set_text(timeLabel, alarm->time);
    lv_obj_set_style_text_color(timeLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(timeLabel, LV_ALIGN_RIGHT_MID, showAckBtn ? -70 : -8, 0);

    // ACK button
    if (showAckBtn && !alarm->acked) {
        lv_obj_t* ackBtn = lv_btn_create(row);
        lv_obj_set_size(ackBtn, 50, 26);
        lv_obj_set_style_bg_color(ackBtn, COLOR_BORDER, 0);
        lv_obj_set_style_radius(ackBtn, 4, 0);
        lv_obj_set_style_shadow_width(ackBtn, 0, 0);
        lv_obj_align(ackBtn, LV_ALIGN_RIGHT_MID, 0, 0);

        lv_obj_t* ackLabel = lv_label_create(ackBtn);
        lv_label_set_text(ackLabel, "ACK");
        lv_obj_set_style_text_color(ackLabel, COLOR_TEXT_PRIMARY, 0);
        lv_obj_center(ackLabel);

        lv_obj_add_event_cb(ackBtn, ack_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)index);
    }

    return row;
}

// ============ Helper: Draw Sparkline from History ============
// 3 static point arrays so each sensor line keeps its own data
static lv_point_precise_t sparklinePoints[3][SENSOR_HISTORY_LEN];

static void draw_sparkline(lv_obj_t* parent, uint8_t sensorIndex, int width, int height, lv_color_t color) {
    SensorHistory_t* hist = sim_get_history(sensorIndex);
    if (!hist || hist->count < 2 || sensorIndex >= 3) return;

    DemoProfile_t* demo = getDemo();
    float sMin = demo->sensors[sensorIndex].min;
    float sMax = demo->sensors[sensorIndex].max;
    float range = sMax - sMin;
    if (range < 0.01f) range = 1.0f;

    int count = hist->count;
    float xStep = (float)width / (float)(count - 1);

    for (int i = 0; i < count; i++) {
        int idx = (hist->head - count + i + SENSOR_HISTORY_LEN) % SENSOR_HISTORY_LEN;
        float val = hist->buffer[idx];
        float norm = (val - sMin) / range;
        if (norm < 0) norm = 0;
        if (norm > 1) norm = 1;

        sparklinePoints[sensorIndex][i].x = (int)(i * xStep);
        sparklinePoints[sensorIndex][i].y = height - (int)(norm * height);
    }

    lv_obj_t* line = lv_line_create(parent);
    lv_line_set_points(line, sparklinePoints[sensorIndex], count);
    lv_obj_set_style_line_color(line, color, 0);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_set_style_line_opa(line, LV_OPA_70, 0);
}

// ============ Helper: Create Alarm Row ============
static lv_obj_t* create_alarm_row(lv_obj_t* parent, Alarm_t* alarm, int index, bool showAckBtn) {
    if (strlen(alarm->message) == 0) return NULL;
    
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(row, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(row, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(row, 8, 0);
    lv_obj_set_size(row, lv_pct(100), 40);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    
    if (alarm->acked) {
        lv_obj_set_style_opa(row, LV_OPA_50, 0);
    }
    
    // Severity dot
    lv_obj_t* dot = lv_obj_create(row);
    lv_obj_set_size(dot, 8, 8);
    lv_obj_set_style_radius(dot, 4, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_align(dot, LV_ALIGN_LEFT_MID, 0, 0);
    
    if (strcmp(alarm->severity, "error") == 0) {
        lv_obj_set_style_bg_color(dot, COLOR_ERROR, 0);
    } else if (strcmp(alarm->severity, "warning") == 0) {
        lv_obj_set_style_bg_color(dot, COLOR_WARNING, 0);
    } else {
        lv_obj_set_style_bg_color(dot, COLOR_INFO, 0);
    }
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    
    // Message
    lv_obj_t* msgLabel = lv_label_create(row);
    lv_label_set_text(msgLabel, alarm->message);
    lv_obj_set_style_text_color(msgLabel, alarm->acked ? COLOR_TEXT_DIM : COLOR_TEXT_PRIMARY, 0);
    lv_obj_align(msgLabel, LV_ALIGN_LEFT_MID, 16, 0);
    
    // Time
    lv_obj_t* timeLabel = lv_label_create(row);
    lv_label_set_text(timeLabel, alarm->time);
    lv_obj_set_style_text_color(timeLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(timeLabel, LV_ALIGN_RIGHT_MID, showAckBtn ? -70 : -8, 0);
    
    // ACK button
    if (showAckBtn && !alarm->acked) {
        lv_obj_t* ackBtn = lv_btn_create(row);
        lv_obj_set_size(ackBtn, 50, 26);
        lv_obj_set_style_bg_color(ackBtn, COLOR_BORDER, 0);
        lv_obj_set_style_radius(ackBtn, 4, 0);
        lv_obj_set_style_shadow_width(ackBtn, 0, 0);
        lv_obj_align(ackBtn, LV_ALIGN_RIGHT_MID, 0, 0);
        
        lv_obj_t* ackLabel = lv_label_create(ackBtn);
        lv_label_set_text(ackLabel, "ACK");
        lv_obj_set_style_text_color(ackLabel, COLOR_TEXT_PRIMARY, 0);
        lv_obj_center(ackLabel);
        
        lv_obj_add_event_cb(ackBtn, ack_btn_event_cb, LV_EVENT_CLICKED, (void*)(intptr_t)index);
    }
    
    return row;
}

// ============ Screen Creation Functions ============

static void create_home_screen(void) {
    screens[SCREEN_HOME] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_HOME], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_HOME], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_HOME], 0, 0);
    lv_obj_set_size(screens[SCREEN_HOME], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_HOME], 0, 0);
    lv_obj_add_flag(screens[SCREEN_HOME], LV_OBJ_FLAG_HIDDEN);
    
    homeContent = lv_obj_create(screens[SCREEN_HOME]);
    lv_obj_set_style_bg_opa(homeContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(homeContent, 0, 0);
    lv_obj_set_style_pad_all(homeContent, 0, 0);
    lv_obj_set_size(homeContent, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(homeContent, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(homeContent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(homeContent, 12, 0);
    
    rebuild_home_content();
}

static void rebuild_home_content(void) {
    if (!homeContent) return;
    lv_obj_clean(homeContent);
    
    DemoProfile_t* demo = getDemo();
    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;
    
    // KPI Row - 4 cards
    lv_obj_t* kpiRow = lv_obj_create(homeContent);
    lv_obj_set_style_bg_opa(kpiRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(kpiRow, 0, 0);
    lv_obj_set_style_pad_all(kpiRow, 0, 0);
    lv_obj_set_size(kpiRow, contentWidth, 75);
    lv_obj_set_layout(kpiRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(kpiRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(kpiRow, 10, 0);
    lv_obj_clear_flag(kpiRow, LV_OBJ_FLAG_SCROLLABLE);
    
    int kpiWidth = (contentWidth - 30) / 4;
    for (int i = 0; i < 4; i++) {
        create_kpi_card(kpiRow, &demo->kpis[i], kpiWidth);
    }
    
    // Sensors Row - 3 cards
    lv_obj_t* sensorRow = lv_obj_create(homeContent);
    lv_obj_set_style_bg_opa(sensorRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sensorRow, 0, 0);
    lv_obj_set_style_pad_all(sensorRow, 0, 0);
    lv_obj_set_size(sensorRow, contentWidth, 100);
    lv_obj_set_layout(sensorRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sensorRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(sensorRow, 10, 0);
    lv_obj_clear_flag(sensorRow, LV_OBJ_FLAG_SCROLLABLE);
    
    int sensorWidth = (contentWidth - 20) / 3;
    for (int i = 0; i < 3; i++) {
        create_sensor_card(sensorRow, &demo->sensors[i], sensorWidth);
    }
    
    // Bottom Row - Alarms and Vision
    lv_obj_t* bottomRow = lv_obj_create(homeContent);
    lv_obj_set_style_bg_opa(bottomRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottomRow, 0, 0);
    lv_obj_set_style_pad_all(bottomRow, 0, 0);
    lv_obj_set_size(bottomRow, contentWidth, 280);
    lv_obj_set_layout(bottomRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottomRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bottomRow, 10, 0);
    lv_obj_clear_flag(bottomRow, LV_OBJ_FLAG_SCROLLABLE);
    
    // Alarms panel
    lv_obj_t* alarmsPanel = lv_obj_create(bottomRow);
    style_card(alarmsPanel);
    lv_obj_set_size(alarmsPanel, (contentWidth - 10) / 2, 270);
    lv_obj_set_layout(alarmsPanel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(alarmsPanel, LV_FLEX_FLOW_COLUMN);
    
    lv_obj_t* alarmsTitle = lv_label_create(alarmsPanel);
    char alarmTitleStr[40];
    uint8_t alarmCount = sim_get_alarm_count();
    snprintf(alarmTitleStr, sizeof(alarmTitleStr), "Live Alarms (%d)", alarmCount);
    lv_label_set_text(alarmsTitle, alarmTitleStr);
    lv_obj_set_style_text_color(alarmsTitle, alarmCount > 0 ? COLOR_WARNING : COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(alarmsTitle, &lv_font_montserrat_14, 0);

    // Show dynamic alarms from simulation engine (most recent first, max 4)
    int shown = 0;
    for (int i = alarmCount - 1; i >= 0 && shown < 4; i--) {
        DynamicAlarm_t* a = sim_get_alarm(i);
        if (a) {
            create_dynamic_alarm_row(alarmsPanel, a, i, false);
            shown++;
        }
    }
    if (shown == 0) {
        lv_obj_t* noAlarms = lv_label_create(alarmsPanel);
        lv_label_set_text(noAlarms, "No active alarms");
        lv_obj_set_style_text_color(noAlarms, COLOR_TEXT_DIM, 0);
    }
    
    // Vision preview panel
    lv_obj_t* visionPanel = lv_obj_create(bottomRow);
    lv_obj_set_style_bg_color(visionPanel, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(visionPanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(visionPanel, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(visionPanel, 1, 0);
    lv_obj_set_style_radius(visionPanel, 8, 0);
    lv_obj_set_style_pad_all(visionPanel, 12, 0);
    lv_obj_set_size(visionPanel, (contentWidth - 10) / 2, 270);
    lv_obj_clear_flag(visionPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* visionTitle = lv_label_create(visionPanel);
    lv_label_set_text(visionTitle, demo->name);
    lv_obj_set_style_text_color(visionTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(visionTitle, &lv_font_montserrat_16, 0);
    lv_obj_align(visionTitle, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t* visionSub = lv_label_create(visionPanel);
    lv_label_set_text(visionSub, demo->sub);
    lv_obj_set_style_text_color(visionSub, COLOR_TEXT_MUTED, 0);
    lv_obj_align(visionSub, LV_ALIGN_TOP_MID, 0, 18);
    
    // Vision content based on demo type
    create_vision_panel_content(visionPanel, demo, 40);
}

// ============ Vision Panel Content ============
static void create_vision_panel_content(lv_obj_t* parent, DemoProfile_t* demo, int yOffset) {
    Vision_t* v = &demo->vision;
    
    if (v->type == VISION_CNC) {
        // Part counter display
        lv_obj_t* partDisplay = lv_obj_create(parent);
        lv_obj_set_style_bg_color(partDisplay, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(partDisplay, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(partDisplay, COLOR_BORDER_LIGHT, 0);
        lv_obj_set_style_border_width(partDisplay, 1, 0);
        lv_obj_set_style_radius(partDisplay, 6, 0);
        lv_obj_set_style_pad_all(partDisplay, 8, 0);
        lv_obj_set_size(partDisplay, 100, 60);
        lv_obj_set_pos(partDisplay, 20, yOffset + 10);
        lv_obj_clear_flag(partDisplay, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* partLabel = lv_label_create(partDisplay);
        lv_label_set_text(partLabel, "PARTS");
        lv_obj_set_style_text_color(partLabel, COLOR_TEXT_DIM, 0);
        lv_obj_align(partLabel, LV_ALIGN_TOP_MID, 0, 0);
        
        char partStr[16];
        snprintf(partStr, sizeof(partStr), "%04d", v->partCount);
        lv_obj_t* partVal = lv_label_create(partDisplay);
        lv_label_set_text(partVal, partStr);
        lv_obj_set_style_text_color(partVal, COLOR_SUCCESS, 0);
        lv_obj_set_style_text_font(partVal, &lv_font_montserrat_24, 0);
        lv_obj_align(partVal, LV_ALIGN_BOTTOM_MID, 0, 0);
        
        // Stack light
        lv_obj_t* stackContainer = lv_obj_create(parent);
        lv_obj_set_style_bg_opa(stackContainer, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(stackContainer, 0, 0);
        lv_obj_set_size(stackContainer, 40, 100);
        lv_obj_set_pos(stackContainer, 150, yOffset);
        lv_obj_clear_flag(stackContainer, LV_OBJ_FLAG_SCROLLABLE);
        
        const char* colors[] = {"red", "yellow", "green"};
        const lv_color_t lvColors[] = {COLOR_ERROR, COLOR_WARNING, COLOR_SUCCESS};
        for (int i = 0; i < 3; i++) {
            lv_obj_t* lamp = lv_obj_create(stackContainer);
            lv_obj_set_size(lamp, 26, 26);
            lv_obj_set_style_radius(lamp, 13, 0);
            lv_obj_set_style_border_width(lamp, 2, 0);
            lv_obj_set_pos(lamp, 5, i * 30);
            
            bool isOn = (strcmp(v->stackLight, colors[i]) == 0);
            if (isOn) {
                lv_obj_set_style_bg_color(lamp, lvColors[i], 0);
                lv_obj_set_style_border_color(lamp, lvColors[i], 0);
                lv_obj_set_style_shadow_color(lamp, lvColors[i], 0);
                lv_obj_set_style_shadow_width(lamp, 10, 0);
            } else {
                lv_obj_set_style_bg_color(lamp, COLOR_BORDER, 0);
                lv_obj_set_style_border_color(lamp, COLOR_BORDER_LIGHT, 0);
            }
            lv_obj_set_style_bg_opa(lamp, LV_OPA_COVER, 0);
        }
        
        // LED indicators
        lv_obj_t* ledPanel = lv_obj_create(parent);
        lv_obj_set_style_bg_opa(ledPanel, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ledPanel, 0, 0);
        lv_obj_set_size(ledPanel, 150, 100);
        lv_obj_set_pos(ledPanel, 210, yOffset);
        lv_obj_set_layout(ledPanel, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(ledPanel, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_style_pad_all(ledPanel, 4, 0);
        lv_obj_set_style_pad_row(ledPanel, 6, 0);
        lv_obj_set_style_pad_column(ledPanel, 10, 0);
        lv_obj_clear_flag(ledPanel, LV_OBJ_FLAG_SCROLLABLE);
        
        const char* ledNames[] = {"RUN", "FEED", "SPIN", "COOL", "PROG", "ERR", "FLT", "RDY"};
        bool ledStates[] = {v->leds.run, v->leds.feed, v->leds.spindle, v->leds.coolant, 
                           v->leds.program, v->leds.error, v->leds.fault, v->leds.ready};
        for (int i = 0; i < 8; i++) {
            lv_obj_t* ledItem = lv_obj_create(ledPanel);
            lv_obj_set_style_bg_opa(ledItem, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(ledItem, 0, 0);
            lv_obj_set_style_pad_all(ledItem, 0, 0);
            lv_obj_set_size(ledItem, 30, 24);
            lv_obj_clear_flag(ledItem, LV_OBJ_FLAG_SCROLLABLE);
            
            lv_obj_t* led = lv_obj_create(ledItem);
            lv_obj_set_size(led, 12, 12);
            lv_obj_set_style_radius(led, 6, 0);
            lv_obj_set_style_border_width(led, 1, 0);
            lv_obj_set_style_border_color(led, COLOR_BORDER_LIGHT, 0);
            lv_obj_set_pos(led, 9, 0);
            
            if (ledStates[i]) {
                lv_color_t c = (i == 5 || i == 6) ? COLOR_ERROR : COLOR_SUCCESS;
                lv_obj_set_style_bg_color(led, c, 0);
                lv_obj_set_style_shadow_color(led, c, 0);
                lv_obj_set_style_shadow_width(led, 6, 0);
            } else {
                lv_obj_set_style_bg_color(led, COLOR_BORDER, 0);
            }
            lv_obj_set_style_bg_opa(led, LV_OPA_COVER, 0);
            
            lv_obj_t* ledLabel = lv_label_create(ledItem);
            lv_label_set_text(ledLabel, ledNames[i]);
            lv_obj_set_style_text_color(ledLabel, COLOR_TEXT_DIM, 0);
            lv_obj_set_pos(ledLabel, 0, 14);
        }
        
    } else if (v->type == VISION_CHILLER) {
        // Error code display
        lv_obj_t* errDisplay = lv_obj_create(parent);
        lv_obj_set_style_bg_color(errDisplay, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(errDisplay, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(errDisplay, COLOR_BORDER_LIGHT, 0);
        lv_obj_set_style_border_width(errDisplay, 1, 0);
        lv_obj_set_style_radius(errDisplay, 6, 0);
        lv_obj_set_style_pad_all(errDisplay, 12, 0);
        lv_obj_set_size(errDisplay, 140, 70);
        lv_obj_align(errDisplay, LV_ALIGN_CENTER, 0, yOffset/2);
        lv_obj_clear_flag(errDisplay, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* errLabel = lv_label_create(errDisplay);
        lv_label_set_text(errLabel, "ERROR CODE");
        lv_obj_set_style_text_color(errLabel, COLOR_TEXT_DIM, 0);
        lv_obj_align(errLabel, LV_ALIGN_TOP_MID, 0, 0);
        
        lv_obj_t* errVal = lv_label_create(errDisplay);
        lv_label_set_text(errVal, v->errorCode);
        bool hasError = strcmp(v->errorCode, "---") != 0;
        lv_obj_set_style_text_color(errVal, hasError ? COLOR_ERROR : COLOR_SUCCESS, 0);
        lv_obj_set_style_text_font(errVal, &lv_font_montserrat_28, 0);
        lv_obj_align(errVal, LV_ALIGN_BOTTOM_MID, 0, 0);
        
    } else if (v->type == VISION_COMPRESSOR) {
        // Pressure gauge
        lv_obj_t* pressBox = lv_obj_create(parent);
        lv_obj_set_style_bg_color(pressBox, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(pressBox, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(pressBox, COLOR_BORDER_LIGHT, 0);
        lv_obj_set_style_border_width(pressBox, 1, 0);
        lv_obj_set_style_radius(pressBox, 6, 0);
        lv_obj_set_style_pad_all(pressBox, 8, 0);
        lv_obj_set_size(pressBox, 100, 55);
        lv_obj_set_pos(pressBox, 30, yOffset + 20);
        lv_obj_clear_flag(pressBox, LV_OBJ_FLAG_SCROLLABLE);
        
        char pressStr[16];
        snprintf(pressStr, sizeof(pressStr), "%.1f", v->pressure);
        lv_obj_t* pressVal = lv_label_create(pressBox);
        lv_label_set_text(pressVal, pressStr);
        lv_obj_set_style_text_color(pressVal, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(pressVal, &lv_font_montserrat_24, 0);
        lv_obj_align(pressVal, LV_ALIGN_TOP_MID, 0, 0);
        
        lv_obj_t* pressLabel = lv_label_create(pressBox);
        lv_label_set_text(pressLabel, "bar");
        lv_obj_set_style_text_color(pressLabel, COLOR_TEXT_DIM, 0);
        lv_obj_align(pressLabel, LV_ALIGN_BOTTOM_MID, 0, 0);
        
        // State indicator
        lv_obj_t* stateBox = lv_obj_create(parent);
        lv_obj_set_style_bg_color(stateBox, lv_color_hex(0x000000), 0);
        lv_obj_set_style_bg_opa(stateBox, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(stateBox, COLOR_BORDER_LIGHT, 0);
        lv_obj_set_style_border_width(stateBox, 1, 0);
        lv_obj_set_style_radius(stateBox, 6, 0);
        lv_obj_set_style_pad_all(stateBox, 8, 0);
        lv_obj_set_size(stateBox, 90, 40);
        lv_obj_set_pos(stateBox, 160, yOffset + 30);
        lv_obj_clear_flag(stateBox, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* stateVal = lv_label_create(stateBox);
        lv_label_set_text(stateVal, v->state);
        bool isLoad = strcmp(v->state, "LOAD") == 0;
        lv_obj_set_style_text_color(stateVal, isLoad ? COLOR_SUCCESS : COLOR_WARNING, 0);
        lv_obj_set_style_text_font(stateVal, &lv_font_montserrat_18, 0);
        lv_obj_center(stateVal);
        
    } else if (v->type == VISION_CUSTOM) {
        // DI panel
        lv_obj_t* diPanel = lv_obj_create(parent);
        lv_obj_set_style_bg_color(diPanel, COLOR_BG_DARK2, 0);
        lv_obj_set_style_bg_opa(diPanel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(diPanel, COLOR_BORDER, 0);
        lv_obj_set_style_border_width(diPanel, 1, 0);
        lv_obj_set_style_radius(diPanel, 4, 0);
        lv_obj_set_style_pad_all(diPanel, 6, 0);
        lv_obj_set_size(diPanel, 170, 50);
        lv_obj_set_pos(diPanel, 10, yOffset + 10);
        lv_obj_clear_flag(diPanel, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* diLabel = lv_label_create(diPanel);
        lv_label_set_text(diLabel, "DI 0.0-0.7");
        lv_obj_set_style_text_color(diLabel, COLOR_TEXT_DIM, 0);
        lv_obj_set_pos(diLabel, 0, 0);
        
        for (int i = 0; i < 8; i++) {
            lv_obj_t* led = lv_obj_create(diPanel);
            lv_obj_set_size(led, 12, 12);
            lv_obj_set_style_radius(led, 6, 0);
            lv_obj_set_style_border_width(led, 0, 0);
            lv_obj_set_pos(led, i * 18 + 10, 22);
            lv_obj_set_style_bg_color(led, v->diA[i] ? COLOR_SUCCESS : COLOR_BORDER, 0);
            lv_obj_set_style_bg_opa(led, LV_OPA_COVER, 0);
            if (v->diA[i]) {
                lv_obj_set_style_shadow_color(led, COLOR_SUCCESS, 0);
                lv_obj_set_style_shadow_width(led, 6, 0);
            }
        }
        
        // DQ panel
        lv_obj_t* dqPanel = lv_obj_create(parent);
        lv_obj_set_style_bg_color(dqPanel, COLOR_BG_DARK2, 0);
        lv_obj_set_style_bg_opa(dqPanel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(dqPanel, COLOR_BORDER, 0);
        lv_obj_set_style_border_width(dqPanel, 1, 0);
        lv_obj_set_style_radius(dqPanel, 4, 0);
        lv_obj_set_style_pad_all(dqPanel, 6, 0);
        lv_obj_set_size(dqPanel, 170, 50);
        lv_obj_set_pos(dqPanel, 190, yOffset + 10);
        lv_obj_clear_flag(dqPanel, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* dqLabel = lv_label_create(dqPanel);
        lv_label_set_text(dqLabel, "DQ 0.0-0.7");
        lv_obj_set_style_text_color(dqLabel, COLOR_TEXT_DIM, 0);
        lv_obj_set_pos(dqLabel, 0, 0);
        
        for (int i = 0; i < 8; i++) {
            lv_obj_t* led = lv_obj_create(dqPanel);
            lv_obj_set_size(led, 12, 12);
            lv_obj_set_style_radius(led, 6, 0);
            lv_obj_set_style_border_width(led, 0, 0);
            lv_obj_set_pos(led, i * 18 + 10, 22);
            lv_obj_set_style_bg_color(led, v->dqA[i] ? COLOR_WARNING : COLOR_BORDER, 0);
            lv_obj_set_style_bg_opa(led, LV_OPA_COVER, 0);
            if (v->dqA[i]) {
                lv_obj_set_style_shadow_color(led, COLOR_WARNING, 0);
                lv_obj_set_style_shadow_width(led, 6, 0);
            }
        }
        
        // AQ0 display
        lv_obj_t* aqPanel = lv_obj_create(parent);
        lv_obj_set_style_bg_color(aqPanel, COLOR_BG_DARK2, 0);
        lv_obj_set_style_bg_opa(aqPanel, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(aqPanel, COLOR_BORDER, 0);
        lv_obj_set_style_border_width(aqPanel, 1, 0);
        lv_obj_set_style_radius(aqPanel, 4, 0);
        lv_obj_set_style_pad_all(aqPanel, 8, 0);
        lv_obj_set_size(aqPanel, 80, 60);
        lv_obj_set_pos(aqPanel, 100, yOffset + 70);
        lv_obj_clear_flag(aqPanel, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* aqLabel = lv_label_create(aqPanel);
        lv_label_set_text(aqLabel, "AQ0");
        lv_obj_set_style_text_color(aqLabel, COLOR_TEXT_DIM, 0);
        lv_obj_align(aqLabel, LV_ALIGN_TOP_MID, 0, 0);
        
        char aqStr[16];
        snprintf(aqStr, sizeof(aqStr), "%d%%", v->aq0);
        lv_obj_t* aqVal = lv_label_create(aqPanel);
        lv_label_set_text(aqVal, aqStr);
        lv_obj_set_style_text_color(aqVal, COLOR_ACCENT, 0);
        lv_obj_set_style_text_font(aqVal, &lv_font_montserrat_18, 0);
        lv_obj_align(aqVal, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

static void create_sensors_screen(void) {
    screens[SCREEN_SENSORS] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_SENSORS], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_SENSORS], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_SENSORS], 0, 0);
    lv_obj_set_size(screens[SCREEN_SENSORS], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_SENSORS], 0, 0);
    lv_obj_add_flag(screens[SCREEN_SENSORS], LV_OBJ_FLAG_HIDDEN);
    
    sensorsContent = lv_obj_create(screens[SCREEN_SENSORS]);
    lv_obj_set_style_bg_opa(sensorsContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(sensorsContent, 0, 0);
    lv_obj_set_style_pad_all(sensorsContent, 0, 0);
    lv_obj_set_size(sensorsContent, lv_pct(100), lv_pct(100));
    
    rebuild_sensors_content();
}

static void rebuild_sensors_content(void) {
    if (!sensorsContent) return;
    lv_obj_clean(sensorsContent);
    
    DemoProfile_t* demo = getDemo();
    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;
    
    lv_obj_t* title = lv_label_create(sensorsContent);
    lv_label_set_text(title, "Sensor Monitoring");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(title, 0, 0);
    
    // Scenario state indicator
    lv_obj_t* scenarioRow = lv_obj_create(sensorsContent);
    lv_obj_set_style_bg_opa(scenarioRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scenarioRow, 0, 0);
    lv_obj_set_style_pad_all(scenarioRow, 0, 0);
    lv_obj_set_size(scenarioRow, contentWidth, 20);
    lv_obj_set_pos(scenarioRow, 0, 25);

    char scenStr[64];
    snprintf(scenStr, sizeof(scenStr), "Scenario: %s", sim_get_scenario_name());
    lv_obj_t* scenLabel = lv_label_create(scenarioRow);
    lv_label_set_text(scenLabel, scenStr);
    ScenarioState_t scState = sim_get_scenario();
    lv_color_t scColor = (scState == SCENARIO_NORMAL) ? COLOR_SUCCESS :
                         (scState == SCENARIO_FAULT) ? COLOR_ERROR :
                         (scState == SCENARIO_RECOVERY) ? COLOR_INFO : COLOR_WARNING;
    lv_obj_set_style_text_color(scenLabel, scColor, 0);

    // Large sensor cards with sparklines
    int sensorWidth = (contentWidth - 20) / 3;
    for (int i = 0; i < 3; i++) {
        lv_obj_t* card = lv_obj_create(sensorsContent);
        style_card(card);
        lv_obj_set_size(card, sensorWidth, 230);
        lv_obj_set_pos(card, i * (sensorWidth + 10), 50);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        Sensor_t* s = &demo->sensors[i];

        lv_obj_t* nameLabel = lv_label_create(card);
        lv_label_set_text(nameLabel, s->name);
        lv_obj_set_style_text_color(nameLabel, COLOR_TEXT_MUTED, 0);
        lv_obj_set_pos(nameLabel, 0, 0);

        lv_obj_t* typeLabel = lv_label_create(card);
        lv_label_set_text(typeLabel, s->type);
        lv_obj_set_style_text_color(typeLabel, COLOR_TEXT_DIM, 0);
        lv_obj_set_pos(typeLabel, 0, 18);

        char valStr[32];
        snprintf(valStr, sizeof(valStr), "%.*f", s->decimals, s->value);
        lv_obj_t* valLabel = lv_label_create(card);
        lv_label_set_text(valLabel, valStr);
        lv_obj_set_style_text_color(valLabel, lv_color_hex(s->color), 0);
        lv_obj_set_style_text_font(valLabel, &lv_font_montserrat_32, 0);
        lv_obj_set_pos(valLabel, 0, 40);

        lv_obj_t* unitLabel = lv_label_create(card);
        lv_label_set_text(unitLabel, s->unit);
        lv_obj_set_style_text_color(unitLabel, COLOR_TEXT_MUTED, 0);
        lv_obj_set_style_text_font(unitLabel, &lv_font_montserrat_18, 0);
        lv_obj_align_to(unitLabel, valLabel, LV_ALIGN_OUT_RIGHT_BOTTOM, 6, 0);

        // Progress bar
        lv_obj_t* bar = lv_bar_create(card);
        lv_obj_set_size(bar, sensorWidth - 24, 8);
        lv_obj_set_pos(bar, 0, 95);
        lv_obj_set_style_bg_color(bar, COLOR_BORDER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, lv_color_hex(s->color), LV_PART_INDICATOR);
        lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);

        int pct = (int)(((s->value - s->min) / (s->max - s->min)) * 100);
        if (pct < 0) pct = 0; if (pct > 100) pct = 100;
        lv_bar_set_value(bar, pct, LV_ANIM_OFF);

        // Sparkline container
        lv_obj_t* sparkBox = lv_obj_create(card);
        lv_obj_set_style_bg_color(sparkBox, COLOR_BG_DARK2, 0);
        lv_obj_set_style_bg_opa(sparkBox, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(sparkBox, COLOR_BORDER, 0);
        lv_obj_set_style_border_width(sparkBox, 1, 0);
        lv_obj_set_style_radius(sparkBox, 4, 0);
        lv_obj_set_style_pad_all(sparkBox, 4, 0);
        lv_obj_set_size(sparkBox, sensorWidth - 24, 70);
        lv_obj_set_pos(sparkBox, 0, 115);
        lv_obj_clear_flag(sparkBox, LV_OBJ_FLAG_SCROLLABLE);

        // Draw sparkline from history
        draw_sparkline(sparkBox, i, sensorWidth - 36, 58, lv_color_hex(s->color));

        // Min/Max labels below sparkline
        char minStr[16], maxStr[16];
        snprintf(minStr, sizeof(minStr), "%.0f", s->min);
        snprintf(maxStr, sizeof(maxStr), "%.0f", s->max);

        lv_obj_t* minLabel = lv_label_create(card);
        lv_label_set_text(minLabel, minStr);
        lv_obj_set_style_text_color(minLabel, COLOR_TEXT_DIM, 0);
        lv_obj_set_pos(minLabel, 0, 190);

        lv_obj_t* maxLabel = lv_label_create(card);
        lv_label_set_text(maxLabel, maxStr);
        lv_obj_set_style_text_color(maxLabel, COLOR_TEXT_DIM, 0);
        lv_obj_align(maxLabel, LV_ALIGN_TOP_RIGHT, 0, 190);
    }
}

static void create_alarms_screen(void) {
    screens[SCREEN_ALARMS] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_ALARMS], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_ALARMS], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_ALARMS], 0, 0);
    lv_obj_set_size(screens[SCREEN_ALARMS], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_ALARMS], 0, 0);
    lv_obj_add_flag(screens[SCREEN_ALARMS], LV_OBJ_FLAG_HIDDEN);
    
    alarmsContent = lv_obj_create(screens[SCREEN_ALARMS]);
    lv_obj_set_style_bg_opa(alarmsContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(alarmsContent, 0, 0);
    lv_obj_set_style_pad_all(alarmsContent, 0, 0);
    lv_obj_set_size(alarmsContent, lv_pct(100), lv_pct(100));
    
    rebuild_alarms_content();
}

static void rebuild_alarms_content(void) {
    if (!alarmsContent) return;
    lv_obj_clean(alarmsContent);

    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;
    uint8_t alarmCount = sim_get_alarm_count();

    // Title with count
    lv_obj_t* title = lv_label_create(alarmsContent);
    char titleStr[48];
    snprintf(titleStr, sizeof(titleStr), "Alarm Management (%d active)", alarmCount);
    lv_label_set_text(title, titleStr);
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(title, 0, 0);

    // Scenario context
    lv_obj_t* scenInfo = lv_label_create(alarmsContent);
    char scenStr[64];
    snprintf(scenStr, sizeof(scenStr), "System State: %s", sim_get_scenario_name());
    lv_label_set_text(scenInfo, scenStr);
    ScenarioState_t scState = sim_get_scenario();
    lv_color_t scColor = (scState == SCENARIO_NORMAL) ? COLOR_SUCCESS :
                         (scState == SCENARIO_FAULT) ? COLOR_ERROR :
                         (scState == SCENARIO_RECOVERY) ? COLOR_INFO : COLOR_WARNING;
    lv_obj_set_style_text_color(scenInfo, scColor, 0);
    lv_obj_set_pos(scenInfo, 0, 28);

    // Dynamic alarms card
    lv_obj_t* alarmsCard = lv_obj_create(alarmsContent);
    style_card(alarmsCard);
    lv_obj_set_size(alarmsCard, contentWidth, 420);
    lv_obj_set_pos(alarmsCard, 0, 50);
    lv_obj_set_layout(alarmsCard, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(alarmsCard, LV_FLEX_FLOW_COLUMN);

    if (alarmCount == 0) {
        lv_obj_t* noAlarms = lv_label_create(alarmsCard);
        lv_label_set_text(noAlarms, "No active alarms - system operating normally");
        lv_obj_set_style_text_color(noAlarms, COLOR_SUCCESS, 0);
    } else {
        // Show most recent alarms first (up to 8)
        for (int i = alarmCount - 1; i >= 0; i--) {
            DynamicAlarm_t* a = sim_get_alarm(i);
            if (a) {
                create_dynamic_alarm_row(alarmsCard, a, i, true);
            }
        }
    }
}

static void create_vision_screen(void) {
    screens[SCREEN_VISION] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_VISION], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_VISION], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_VISION], 0, 0);
    lv_obj_set_size(screens[SCREEN_VISION], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_VISION], 0, 0);
    lv_obj_add_flag(screens[SCREEN_VISION], LV_OBJ_FLAG_HIDDEN);
    
    visionContent = lv_obj_create(screens[SCREEN_VISION]);
    lv_obj_set_style_bg_opa(visionContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(visionContent, 0, 0);
    lv_obj_set_style_pad_all(visionContent, 0, 0);
    lv_obj_set_size(visionContent, lv_pct(100), lv_pct(100));
    
    rebuild_vision_content();
}

static void rebuild_vision_content(void) {
    if (!visionContent) return;
    lv_obj_clean(visionContent);
    
    DemoProfile_t* demo = getDemo();
    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;
    
    lv_obj_t* title = lv_label_create(visionContent);
    lv_label_set_text(title, "Computer Vision");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(title, 0, 0);
    
    // Vision panel
    lv_obj_t* visionPanel = lv_obj_create(visionContent);
    lv_obj_set_style_bg_color(visionPanel, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(visionPanel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(visionPanel, COLOR_BORDER, 0);
    lv_obj_set_style_border_width(visionPanel, 1, 0);
    lv_obj_set_style_radius(visionPanel, 8, 0);
    lv_obj_set_style_pad_all(visionPanel, 16, 0);
    lv_obj_set_size(visionPanel, contentWidth, 350);
    lv_obj_set_pos(visionPanel, 0, 35);
    lv_obj_clear_flag(visionPanel, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* panelTitle = lv_label_create(visionPanel);
    lv_label_set_text(panelTitle, demo->name);
    lv_obj_set_style_text_color(panelTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(panelTitle, &lv_font_montserrat_18, 0);
    lv_obj_align(panelTitle, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_t* panelSub = lv_label_create(visionPanel);
    lv_label_set_text(panelSub, demo->sub);
    lv_obj_set_style_text_color(panelSub, COLOR_TEXT_MUTED, 0);
    lv_obj_align(panelSub, LV_ALIGN_TOP_MID, 0, 22);
    
    create_vision_panel_content(visionPanel, demo, 50);
    
    // Camera status card
    lv_obj_t* camCard = lv_obj_create(visionContent);
    style_card(camCard);
    lv_obj_set_size(camCard, contentWidth, 50);
    lv_obj_set_pos(camCard, 0, 400);
    lv_obj_clear_flag(camCard, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* camLabel = lv_label_create(camCard);
    lv_label_set_text(camLabel, LV_SYMBOL_IMAGE " Camera stream active - 30 FPS - CV processing enabled");
    lv_obj_set_style_text_color(camLabel, COLOR_TEXT_MUTED, 0);
    lv_obj_center(camLabel);
}

// ============ QR Code Generation (Simple visual representation) ============
static void create_qr_code(lv_obj_t* parent, const char* data, int size) {
    // Create a visual QR code pattern (simplified - in production use qrcode library)
    lv_obj_t* qrContainer = lv_obj_create(parent);
    lv_obj_set_size(qrContainer, size, size);
    lv_obj_set_style_bg_color(qrContainer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(qrContainer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(qrContainer, 0, 0);
    lv_obj_set_style_radius(qrContainer, 4, 0);
    lv_obj_set_style_pad_all(qrContainer, 8, 0);
    lv_obj_clear_flag(qrContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(qrContainer);
    
    // Generate pseudo-random pattern based on data hash
    uint32_t hash = 0;
    for (int i = 0; data[i]; i++) {
        hash = hash * 31 + data[i];
    }
    
    int moduleSize = (size - 16) / 21;  // 21x21 QR code grid
    int offset = (size - 16 - moduleSize * 21) / 2 + 8;
    
    // Draw finder patterns (corners)
    for (int corner = 0; corner < 3; corner++) {
        int cx = (corner == 1) ? 14 : 0;
        int cy = (corner == 2) ? 14 : 0;
        
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 7; x++) {
                bool black = (y == 0 || y == 6 || x == 0 || x == 6 || 
                             (x >= 2 && x <= 4 && y >= 2 && y <= 4));
                if (black) {
                    lv_obj_t* module = lv_obj_create(qrContainer);
                    lv_obj_set_size(module, moduleSize, moduleSize);
                    lv_obj_set_pos(module, offset + (cx + x) * moduleSize, offset + (cy + y) * moduleSize);
                    lv_obj_set_style_bg_color(module, lv_color_hex(0x000000), 0);
                    lv_obj_set_style_bg_opa(module, LV_OPA_COVER, 0);
                    lv_obj_set_style_border_width(module, 0, 0);
                    lv_obj_set_style_radius(module, 0, 0);
                }
            }
        }
    }
    
    // Draw data modules (pseudo-random pattern)
    for (int y = 0; y < 21; y++) {
        for (int x = 0; x < 21; x++) {
            // Skip finder patterns
            if ((x < 8 && y < 8) || (x > 12 && y < 8) || (x < 8 && y > 12)) continue;
            
            // Use hash to determine if module is black
            uint32_t bit = (hash >> ((x + y * 21) % 32)) & 1;
            if (bit) {
                lv_obj_t* module = lv_obj_create(qrContainer);
                lv_obj_set_size(module, moduleSize, moduleSize);
                lv_obj_set_pos(module, offset + x * moduleSize, offset + y * moduleSize);
                lv_obj_set_style_bg_color(module, lv_color_hex(0x000000), 0);
                lv_obj_set_style_bg_opa(module, LV_OPA_COVER, 0);
                lv_obj_set_style_border_width(module, 0, 0);
                lv_obj_set_style_radius(module, 0, 0);
            }
        }
    }
}

// ============ AI Insight Card Helper ============
static void create_insight_card(lv_obj_t* parent, AIInsight_t* insight, int width) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, width, 85);
    lv_obj_set_style_bg_color(card, COLOR_BG_DARK2, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 6, 0);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    
    // Severity indicator
    lv_color_t sevColor = (insight->severity == INSIGHT_CRITICAL) ? COLOR_ERROR :
                          (insight->severity == INSIGHT_WARNING) ? COLOR_WARNING : COLOR_SUCCESS;
    lv_obj_set_style_border_color(card, sevColor, 0);
    lv_obj_set_style_border_side(card, LV_BORDER_SIDE_LEFT, 0);
    lv_obj_set_style_border_width(card, 3, 0);
    
    // Title
    lv_obj_t* titleLabel = lv_label_create(card);
    lv_label_set_text(titleLabel, insight->title);
    lv_obj_set_style_text_color(titleLabel, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(titleLabel, 0, 0);
    
    // Description
    lv_obj_t* descLabel = lv_label_create(card);
    lv_label_set_text(descLabel, insight->description);
    lv_obj_set_style_text_color(descLabel, COLOR_TEXT_MUTED, 0);
    lv_label_set_long_mode(descLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(descLabel, width - 100);
    lv_obj_set_pos(descLabel, 0, 20);
    
    // Confidence badge
    char confStr[16];
    snprintf(confStr, sizeof(confStr), "%d%%", insight->confidence);
    lv_obj_t* confBadge = lv_obj_create(card);
    lv_obj_set_size(confBadge, 40, 22);
    lv_obj_set_style_bg_color(confBadge, sevColor, 0);
    lv_obj_set_style_bg_opa(confBadge, LV_OPA_30, 0);
    lv_obj_set_style_border_width(confBadge, 0, 0);
    lv_obj_set_style_radius(confBadge, 4, 0);
    lv_obj_align(confBadge, LV_ALIGN_TOP_RIGHT, 0, 0);
    
    lv_obj_t* confLabel = lv_label_create(confBadge);
    lv_label_set_text(confLabel, confStr);
    lv_obj_set_style_text_color(confLabel, sevColor, 0);
    lv_obj_center(confLabel);
    
    // Timeframe
    lv_obj_t* timeLabel = lv_label_create(card);
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), LV_SYMBOL_LOOP " %s", insight->timeframe);
    lv_label_set_text(timeLabel, timeStr);
    lv_obj_set_style_text_color(timeLabel, COLOR_TEXT_DIM, 0);
    lv_obj_align(timeLabel, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
}

// ============ AI Screen ============
static void create_ai_screen(void) {
    screens[SCREEN_AI] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_AI], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_AI], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_AI], 0, 0);
    lv_obj_set_size(screens[SCREEN_AI], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_AI], 0, 0);
    lv_obj_add_flag(screens[SCREEN_AI], LV_OBJ_FLAG_HIDDEN);
    
    aiContent = lv_obj_create(screens[SCREEN_AI]);
    lv_obj_set_style_bg_opa(aiContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(aiContent, 0, 0);
    lv_obj_set_style_pad_all(aiContent, 0, 0);
    lv_obj_set_size(aiContent, lv_pct(100), lv_pct(100));
    
    rebuild_ai_content();
}

static void rebuild_ai_content(void) {
    if (!aiContent) return;
    lv_obj_clean(aiContent);
    
    DemoProfile_t* demo = getDemo();
    AIState_t* ai = &demo->ai;
    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;
    
    // Title
    lv_obj_t* title = lv_label_create(aiContent);
    lv_label_set_text(title, "AI Predictive Maintenance");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(title, 0, 0);
    
    // Model status badge
    lv_obj_t* modelBadge = lv_obj_create(aiContent);
    lv_obj_set_size(modelBadge, 100, 26);
    bool isLearning = strcmp(ai->modelStatus, "Learning") == 0;
    lv_obj_set_style_bg_color(modelBadge, isLearning ? COLOR_WARNING : COLOR_SUCCESS, 0);
    lv_obj_set_style_bg_opa(modelBadge, LV_OPA_30, 0);
    lv_obj_set_style_border_width(modelBadge, 0, 0);
    lv_obj_set_style_radius(modelBadge, 4, 0);
    lv_obj_set_pos(modelBadge, contentWidth - 100, 0);
    
    lv_obj_t* modelLabel = lv_label_create(modelBadge);
    lv_label_set_text(modelLabel, ai->modelStatus);
    lv_obj_set_style_text_color(modelLabel, isLearning ? COLOR_WARNING : COLOR_SUCCESS, 0);
    lv_obj_center(modelLabel);
    
    // ========== Top Row: Health Score + Quick Stats ==========
    lv_obj_t* topRow = lv_obj_create(aiContent);
    lv_obj_set_style_bg_opa(topRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(topRow, 0, 0);
    lv_obj_set_style_pad_all(topRow, 0, 0);
    lv_obj_set_size(topRow, contentWidth, 140);
    lv_obj_set_pos(topRow, 0, 35);
    lv_obj_set_layout(topRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(topRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(topRow, 10, 0);
    lv_obj_clear_flag(topRow, LV_OBJ_FLAG_SCROLLABLE);
    
    // Health Score Card (large circular gauge)
    lv_obj_t* healthCard = lv_obj_create(topRow);
    style_card(healthCard);
    lv_obj_set_size(healthCard, 160, 130);
    lv_obj_clear_flag(healthCard, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* healthTitle = lv_label_create(healthCard);
    lv_label_set_text(healthTitle, "Health Score");
    lv_obj_set_style_text_color(healthTitle, COLOR_TEXT_MUTED, 0);
    lv_obj_align(healthTitle, LV_ALIGN_TOP_MID, 0, 0);
    
    // Health arc
    lv_obj_t* arc = lv_arc_create(healthCard);
    lv_obj_set_size(arc, 80, 80);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_value(arc, ai->healthScore);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, COLOR_BORDER, LV_PART_MAIN);
    lv_color_t healthColor = (ai->healthScore >= 80) ? COLOR_SUCCESS : 
                             (ai->healthScore >= 60) ? COLOR_WARNING : COLOR_ERROR;
    lv_obj_set_style_arc_color(arc, healthColor, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 10);
    
    char healthStr[8];
    snprintf(healthStr, sizeof(healthStr), "%d", ai->healthScore);
    lv_obj_t* healthVal = lv_label_create(arc);
    lv_label_set_text(healthVal, healthStr);
    lv_obj_set_style_text_color(healthVal, healthColor, 0);
    lv_obj_set_style_text_font(healthVal, &lv_font_montserrat_24, 0);
    lv_obj_center(healthVal);
    
    // Quick Stats Cards
    const char* statLabels[] = {"Failure Risk", "Anomalies", "Data Points", "Next Maint."};
    char statValues[4][32];
    snprintf(statValues[0], sizeof(statValues[0]), "%.1f%%", ai->failureProbability);
    snprintf(statValues[1], sizeof(statValues[1]), "%d", ai->anomalyCount);
    snprintf(statValues[2], sizeof(statValues[2]), "%lu", (unsigned long)ai->dataPoints);
    snprintf(statValues[3], sizeof(statValues[3]), "%s", ai->nextMaintenance);
    
    lv_color_t statColors[] = {
        (ai->failureProbability > 25) ? COLOR_ERROR : (ai->failureProbability > 10) ? COLOR_WARNING : COLOR_SUCCESS,
        (ai->anomalyCount > 1) ? COLOR_WARNING : COLOR_SUCCESS,
        COLOR_ACCENT,
        COLOR_INFO
    };
    
    int statWidth = (contentWidth - 160 - 40) / 4;
    for (int i = 0; i < 4; i++) {
        lv_obj_t* statCard = lv_obj_create(topRow);
        style_card(statCard);
        lv_obj_set_size(statCard, statWidth, 130);
        lv_obj_clear_flag(statCard, LV_OBJ_FLAG_SCROLLABLE);
        
        lv_obj_t* sLabel = lv_label_create(statCard);
        lv_label_set_text(sLabel, statLabels[i]);
        lv_obj_set_style_text_color(sLabel, COLOR_TEXT_MUTED, 0);
        lv_obj_align(sLabel, LV_ALIGN_TOP_MID, 0, 0);
        
        lv_obj_t* sVal = lv_label_create(statCard);
        lv_label_set_text(sVal, statValues[i]);
        lv_obj_set_style_text_color(sVal, statColors[i], 0);
        lv_obj_set_style_text_font(sVal, (i == 2) ? &lv_font_montserrat_16 : &lv_font_montserrat_24, 0);
        lv_obj_center(sVal);
    }
    
    // ========== Middle Row: Predictions ==========
    lv_obj_t* predTitle = lv_label_create(aiContent);
    lv_label_set_text(predTitle, LV_SYMBOL_WARNING " Active Predictions");
    lv_obj_set_style_text_color(predTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(predTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(predTitle, 0, 185);
    
    lv_obj_t* predRow = lv_obj_create(aiContent);
    lv_obj_set_style_bg_opa(predRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(predRow, 0, 0);
    lv_obj_set_style_pad_all(predRow, 0, 0);
    lv_obj_set_size(predRow, contentWidth, 95);
    lv_obj_set_pos(predRow, 0, 210);
    lv_obj_set_layout(predRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(predRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(predRow, 10, 0);
    lv_obj_clear_flag(predRow, LV_OBJ_FLAG_SCROLLABLE);
    
    int insightWidth = (contentWidth - 20) / 3;
    for (int i = 0; i < 3; i++) {
        create_insight_card(predRow, &ai->insights[i], insightWidth);
    }
    
    // ========== Bottom Row: QR Code + OTA ==========
    lv_obj_t* bottomRow = lv_obj_create(aiContent);
    lv_obj_set_style_bg_opa(bottomRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottomRow, 0, 0);
    lv_obj_set_style_pad_all(bottomRow, 0, 0);
    lv_obj_set_size(bottomRow, contentWidth, 160);
    lv_obj_set_pos(bottomRow, 0, 315);
    lv_obj_set_layout(bottomRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottomRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bottomRow, 10, 0);
    lv_obj_clear_flag(bottomRow, LV_OBJ_FLAG_SCROLLABLE);
    
    // QR Code Card
    lv_obj_t* qrCard = lv_obj_create(bottomRow);
    style_card(qrCard);
    lv_obj_set_size(qrCard, 200, 150);
    lv_obj_clear_flag(qrCard, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* qrTitle = lv_label_create(qrCard);
    lv_label_set_text(qrTitle, "Remote Dashboard");
    lv_obj_set_style_text_color(qrTitle, COLOR_TEXT_MUTED, 0);
    lv_obj_align(qrTitle, LV_ALIGN_TOP_MID, 0, -4);
    
    // Generate QR code URL
    char qrUrl[160];
    snprintf(qrUrl, sizeof(qrUrl), "%s?device=%s", REMOTE_DASHBOARD_URL, deviceId);
    create_qr_code(qrCard, qrUrl, 100);
    
    lv_obj_t* qrIdLabel = lv_label_create(qrCard);
    lv_label_set_text(qrIdLabel, deviceId);
    lv_obj_set_style_text_color(qrIdLabel, COLOR_ACCENT, 0);
    lv_obj_align(qrIdLabel, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // OTA Update Card
    lv_obj_t* otaCard = lv_obj_create(bottomRow);
    style_card(otaCard);
    lv_obj_set_size(otaCard, contentWidth - 210, 150);
    lv_obj_clear_flag(otaCard, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_t* otaTitle = lv_label_create(otaCard);
    lv_label_set_text(otaTitle, LV_SYMBOL_DOWNLOAD " Firmware Update");
    lv_obj_set_style_text_color(otaTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(otaTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(otaTitle, 0, 0);
    
    lv_obj_t* fwCurrent = lv_label_create(otaCard);
    lv_label_set_text(fwCurrent, "Current: v1.0.0");
    lv_obj_set_style_text_color(fwCurrent, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(fwCurrent, 0, 25);
    
    lv_obj_t* fwAvail = lv_label_create(otaCard);
    lv_label_set_text(fwAvail, "Available: v1.1.0 " LV_SYMBOL_NEW_LINE);
    lv_obj_set_style_text_color(fwAvail, COLOR_SUCCESS, 0);
    lv_obj_set_pos(fwAvail, 0, 45);
    
    // OTA Progress bar
    lv_obj_t* otaProgressBar = lv_bar_create(otaCard);
    lv_obj_set_size(otaProgressBar, contentWidth - 250, 12);
    lv_obj_set_pos(otaProgressBar, 0, 75);
    lv_obj_set_style_bg_color(otaProgressBar, COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(otaProgressBar, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_radius(otaProgressBar, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(otaProgressBar, 6, LV_PART_INDICATOR);

    bool otaActive = sim_ota_active();
    uint8_t otaProg = sim_ota_progress();
    lv_bar_set_value(otaProgressBar, otaActive ? otaProg : 0, LV_ANIM_OFF);

    lv_obj_t* otaStatus = lv_label_create(otaCard);
    if (otaActive) {
        char otaStr[48];
        snprintf(otaStr, sizeof(otaStr), "Downloading firmware... %d%%", otaProg);
        lv_label_set_text(otaStatus, otaStr);
        lv_obj_set_style_text_color(otaStatus, COLOR_ACCENT, 0);
    } else if (otaProg >= 100) {
        lv_label_set_text(otaStatus, "Update complete! Running v1.1.0");
        lv_obj_set_style_text_color(otaStatus, COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(otaStatus, "Ready to update - tap button to start");
        lv_obj_set_style_text_color(otaStatus, COLOR_TEXT_DIM, 0);
    }
    lv_obj_set_pos(otaStatus, 0, 95);

    // Update button
    lv_obj_t* updateBtn = lv_btn_create(otaCard);
    lv_obj_set_size(updateBtn, 130, 35);
    lv_obj_set_style_radius(updateBtn, 6, 0);
    lv_obj_set_style_shadow_width(updateBtn, 0, 0);
    lv_obj_align(updateBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    lv_obj_t* updateLabel = lv_label_create(updateBtn);
    if (otaActive) {
        lv_obj_set_style_bg_color(updateBtn, COLOR_BORDER, 0);
        lv_label_set_text(updateLabel, "Updating...");
        lv_obj_set_style_text_color(updateLabel, COLOR_TEXT_MUTED, 0);
    } else {
        lv_obj_set_style_bg_color(updateBtn, COLOR_ACCENT, 0);
        lv_label_set_text(updateLabel, "Start Update");
        lv_obj_set_style_text_color(updateLabel, COLOR_BG_DARK, 0);
        lv_obj_add_event_cb(updateBtn, ota_btn_event_cb, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_center(updateLabel);
}

// ============ Remote View Screen ============
static void create_remote_screen(void) {
    screens[SCREEN_REMOTE] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_REMOTE], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_REMOTE], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_REMOTE], 0, 0);
    lv_obj_set_size(screens[SCREEN_REMOTE], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_REMOTE], 0, 0);
    lv_obj_add_flag(screens[SCREEN_REMOTE], LV_OBJ_FLAG_HIDDEN);

    remoteContent = lv_obj_create(screens[SCREEN_REMOTE]);
    lv_obj_set_style_bg_opa(remoteContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(remoteContent, 0, 0);
    lv_obj_set_style_pad_all(remoteContent, 0, 0);
    lv_obj_set_size(remoteContent, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(remoteContent, LV_OBJ_FLAG_SCROLLABLE);

    rebuild_remote_content();
}

static void rebuild_remote_content(void) {
    if (!remoteContent) return;
    lv_obj_clean(remoteContent);

    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;

    lv_obj_t* title = lv_label_create(remoteContent);
    lv_label_set_text(title, "Remote View");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(title, 0, 0);

    lv_obj_t* subtitle = lv_label_create(remoteContent);
    lv_label_set_text(subtitle, "Scan to open the dashboard in index.html");
    lv_obj_set_style_text_color(subtitle, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(subtitle, 0, 24);

    lv_obj_t* mainRow = lv_obj_create(remoteContent);
    lv_obj_set_style_bg_opa(mainRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mainRow, 0, 0);
    lv_obj_set_style_pad_all(mainRow, 0, 0);
    lv_obj_set_size(mainRow, contentWidth, 250);
    lv_obj_set_pos(mainRow, 0, 55);
    lv_obj_set_layout(mainRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mainRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(mainRow, 10, 0);
    lv_obj_clear_flag(mainRow, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* qrCard = lv_obj_create(mainRow);
    style_card(qrCard);
    lv_obj_set_size(qrCard, 230, 240);
    lv_obj_clear_flag(qrCard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* qrTitle = lv_label_create(qrCard);
    lv_label_set_text(qrTitle, "Device QR");
    lv_obj_set_style_text_color(qrTitle, COLOR_TEXT_MUTED, 0);
    lv_obj_align(qrTitle, LV_ALIGN_TOP_MID, 0, -2);

    char qrUrl[160];
    snprintf(qrUrl, sizeof(qrUrl), "%s?device=%s", REMOTE_DASHBOARD_URL, deviceId);
    create_qr_code(qrCard, qrUrl, 130);

    lv_obj_t* qrIdLabel = lv_label_create(qrCard);
    lv_label_set_text(qrIdLabel, deviceId);
    lv_obj_set_style_text_color(qrIdLabel, COLOR_ACCENT, 0);
    lv_obj_align(qrIdLabel, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t* infoCard = lv_obj_create(mainRow);
    style_card(infoCard);
    lv_obj_set_size(infoCard, contentWidth - 240, 240);
    lv_obj_clear_flag(infoCard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* infoTitle = lv_label_create(infoCard);
    lv_label_set_text(infoTitle, "Dashboard Link");
    lv_obj_set_style_text_color(infoTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(infoTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(infoTitle, 0, 0);

    lv_obj_t* infoUrl = lv_label_create(infoCard);
    lv_label_set_text(infoUrl, qrUrl);
    lv_obj_set_style_text_color(infoUrl, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(infoUrl, &lv_font_montserrat_14, 0);
    lv_label_set_long_mode(infoUrl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(infoUrl, contentWidth - 280);
    lv_obj_set_pos(infoUrl, 0, 28);

    lv_obj_t* hint = lv_label_create(infoCard);
    lv_label_set_text(hint, "Host index.html at the URL above (local server or GitHub Pages).");
    lv_obj_set_style_text_color(hint, COLOR_TEXT_DIM, 0);
    lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(hint, contentWidth - 280);
    lv_obj_set_pos(hint, 0, 78);

    lv_obj_t* setupStatus = lv_label_create(infoCard);
    if (uiState.setupCompleted) {
        lv_label_set_text(setupStatus, "Setup complete");
        lv_obj_set_style_text_color(setupStatus, COLOR_SUCCESS, 0);
    } else {
        lv_label_set_text(setupStatus, LV_SYMBOL_WARNING " Setup not completed yet");
        lv_obj_set_style_text_color(setupStatus, COLOR_WARNING, 0);
    }
    lv_obj_set_pos(setupStatus, 0, 140);
}

static lv_obj_t* settingsContent = NULL;

static void rebuild_settings_content(void);

static void create_settings_screen(void) {
    screens[SCREEN_SETTINGS] = lv_obj_create(contentArea);
    lv_obj_set_style_bg_opa(screens[SCREEN_SETTINGS], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(screens[SCREEN_SETTINGS], 0, 0);
    lv_obj_set_style_pad_all(screens[SCREEN_SETTINGS], 0, 0);
    lv_obj_set_size(screens[SCREEN_SETTINGS], lv_pct(100), lv_pct(100));
    lv_obj_set_pos(screens[SCREEN_SETTINGS], 0, 0);
    lv_obj_add_flag(screens[SCREEN_SETTINGS], LV_OBJ_FLAG_HIDDEN);

    settingsContent = lv_obj_create(screens[SCREEN_SETTINGS]);
    lv_obj_set_style_bg_opa(settingsContent, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(settingsContent, 0, 0);
    lv_obj_set_style_pad_all(settingsContent, 0, 0);
    lv_obj_set_size(settingsContent, lv_pct(100), lv_pct(100));

    rebuild_settings_content();
}

static void rebuild_settings_content(void) {
    if (!settingsContent) return;
    lv_obj_clean(settingsContent);

    int contentWidth = DISPLAY_WIDTH - SIDEBAR_WIDTH - 28;
    DemoProfile_t* demo = getDemo();
    int halfWidth = (contentWidth - 10) / 2;

    lv_obj_t* title = lv_label_create(settingsContent);
    lv_label_set_text(title, "Device Settings");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(title, 0, 0);

    // ========== Left Column: Device Info ==========
    // Device info card
    lv_obj_t* deviceCard = lv_obj_create(settingsContent);
    style_card(deviceCard);
    lv_obj_set_size(deviceCard, halfWidth, 130);
    lv_obj_set_pos(deviceCard, 0, 35);
    lv_obj_clear_flag(deviceCard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* deviceTitle = lv_label_create(deviceCard);
    lv_label_set_text(deviceTitle, LV_SYMBOL_SETTINGS " Device Information");
    lv_obj_set_style_text_color(deviceTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(deviceTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(deviceTitle, 0, 0);

    const char* infoLabels[] = {"Device ID", "Hardware", "Firmware", "Display"};
    const char* infoValues[] = {"STAP-001-A7F3", "ESP32-P4 / JC1060P470C", "v1.0.0", "7\" 1024x600 MIPI-DSI"};
    for (int i = 0; i < 4; i++) {
        char row[80];
        snprintf(row, sizeof(row), "%s: %s", infoLabels[i], infoValues[i]);
        lv_obj_t* rowLabel = lv_label_create(deviceCard);
        lv_label_set_text(rowLabel, row);
        lv_obj_set_style_text_color(rowLabel, COLOR_TEXT_MUTED, 0);
        lv_obj_set_pos(rowLabel, 0, 25 + i * 22);
    }

    // ========== Right Column: Simulation Info ==========
    lv_obj_t* simCard = lv_obj_create(settingsContent);
    style_card(simCard);
    lv_obj_set_size(simCard, halfWidth, 130);
    lv_obj_set_pos(simCard, halfWidth + 10, 35);
    lv_obj_clear_flag(simCard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* simTitle = lv_label_create(simCard);
    lv_label_set_text(simTitle, LV_SYMBOL_LOOP " Simulation Engine");
    lv_obj_set_style_text_color(simTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(simTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(simTitle, 0, 0);

    SimState_t* simState = sim_get_state();
    char simInfo[80];

    snprintf(simInfo, sizeof(simInfo), "Scenario: %s", sim_get_scenario_name());
    lv_obj_t* scenRow = lv_label_create(simCard);
    lv_label_set_text(scenRow, simInfo);
    ScenarioState_t scState = sim_get_scenario();
    lv_obj_set_style_text_color(scenRow, (scState == SCENARIO_NORMAL) ? COLOR_SUCCESS :
                                          (scState == SCENARIO_FAULT) ? COLOR_ERROR : COLOR_WARNING, 0);
    lv_obj_set_pos(scenRow, 0, 25);

    snprintf(simInfo, sizeof(simInfo), "Cycle: %d | Timer: %lus",
             simState ? simState->cycleCount : 0,
             simState ? simState->stateTimer : 0);
    lv_obj_t* cycleRow = lv_label_create(simCard);
    lv_label_set_text(cycleRow, simInfo);
    lv_obj_set_style_text_color(cycleRow, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(cycleRow, 0, 47);

    snprintf(simInfo, sizeof(simInfo), "Active Demo: %s", demo->name);
    lv_obj_t* demoRow = lv_label_create(simCard);
    lv_label_set_text(demoRow, simInfo);
    lv_obj_set_style_text_color(demoRow, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(demoRow, 0, 69);

    snprintf(simInfo, sizeof(simInfo), "Dynamic Alarms: %d active", sim_get_alarm_count());
    lv_obj_t* alarmRow = lv_label_create(simCard);
    lv_label_set_text(alarmRow, simInfo);
    lv_obj_set_style_text_color(alarmRow, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(alarmRow, 0, 91);

    // ========== Sensor Config Card (full width) ==========
    lv_obj_t* sensorCard = lv_obj_create(settingsContent);
    style_card(sensorCard);
    lv_obj_set_size(sensorCard, contentWidth, 170);
    lv_obj_set_pos(sensorCard, 0, 180);
    lv_obj_clear_flag(sensorCard, LV_OBJ_FLAG_SCROLLABLE);

    char sensorTitleStr[64];
    snprintf(sensorTitleStr, sizeof(sensorTitleStr), LV_SYMBOL_EYE_OPEN " Sensor Configuration (%s)", demo->name);
    lv_obj_t* sensorTitleLabel = lv_label_create(sensorCard);
    lv_label_set_text(sensorTitleLabel, sensorTitleStr);
    lv_obj_set_style_text_color(sensorTitleLabel, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(sensorTitleLabel, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(sensorTitleLabel, 0, 0);

    for (int i = 0; i < 3; i++) {
        Sensor_t* s = &demo->sensors[i];

        lv_obj_t* row = lv_obj_create(sensorCard);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(row, COLOR_BORDER, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_pad_all(row, 8, 0);
        lv_obj_set_size(row, contentWidth - 24, 38);
        lv_obj_set_pos(row, 0, 28 + i * 42);
        lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        // Color dot
        lv_obj_t* cdot = lv_obj_create(row);
        lv_obj_set_size(cdot, 10, 10);
        lv_obj_set_style_radius(cdot, 5, 0);
        lv_obj_set_style_bg_color(cdot, lv_color_hex(s->color), 0);
        lv_obj_set_style_bg_opa(cdot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(cdot, 0, 0);
        lv_obj_align(cdot, LV_ALIGN_LEFT_MID, 0, 0);

        lv_obj_t* sName = lv_label_create(row);
        lv_label_set_text(sName, s->name);
        lv_obj_set_style_text_color(sName, COLOR_TEXT_PRIMARY, 0);
        lv_obj_align(sName, LV_ALIGN_LEFT_MID, 18, -8);

        char sInfo[80];
        snprintf(sInfo, sizeof(sInfo), "%s | Range: %.0f - %.0f %s", s->type, s->min, s->max, s->unit);
        lv_obj_t* sInfoLabel = lv_label_create(row);
        lv_label_set_text(sInfoLabel, sInfo);
        lv_obj_set_style_text_color(sInfoLabel, COLOR_TEXT_DIM, 0);
        lv_obj_align(sInfoLabel, LV_ALIGN_LEFT_MID, 18, 8);

        // Current value
        char valStr[32];
        snprintf(valStr, sizeof(valStr), "%.*f %s", s->decimals, s->value, s->unit);
        lv_obj_t* valLabel = lv_label_create(row);
        lv_label_set_text(valLabel, valStr);
        lv_obj_set_style_text_color(valLabel, lv_color_hex(s->color), 0);
        lv_obj_align(valLabel, LV_ALIGN_RIGHT_MID, -30, 0);

        // Status dot
        lv_obj_t* dot = lv_obj_create(row);
        lv_obj_set_size(dot, 8, 8);
        lv_obj_set_style_radius(dot, 4, 0);
        lv_obj_set_style_bg_color(dot, COLOR_SUCCESS, 0);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_align(dot, LV_ALIGN_RIGHT_MID, 0, 0);
    }

    // ========== About Card ==========
    lv_obj_t* aboutCard = lv_obj_create(settingsContent);
    style_card(aboutCard);
    lv_obj_set_size(aboutCard, contentWidth, 100);
    lv_obj_set_pos(aboutCard, 0, 365);
    lv_obj_clear_flag(aboutCard, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* aboutTitle = lv_label_create(aboutCard);
    lv_label_set_text(aboutTitle, LV_SYMBOL_HOME " About SIGNALTAP");
    lv_obj_set_style_text_color(aboutTitle, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(aboutTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(aboutTitle, 0, 0);

    lv_obj_t* aboutDesc = lv_label_create(aboutCard);
    lv_label_set_text(aboutDesc, "Industrial IoT Retrofit Solution - Non-invasive monitoring\n"
                                  "for existing industrial equipment via computer vision & AI.\n"
                                  "Scenario Engine v2.0 | LVGL 9.2.2 | ESP32-P4");
    lv_obj_set_style_text_color(aboutDesc, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(aboutDesc, 0, 25);
    lv_label_set_long_mode(aboutDesc, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(aboutDesc, contentWidth - 24);
}
