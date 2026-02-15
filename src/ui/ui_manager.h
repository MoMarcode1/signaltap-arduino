// SIGNALTAP UI Manager
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <lvgl.h>
#include "../data/demo_profiles.h"

// ============ Screen IDs ============
typedef enum {
    SCREEN_SPLASH = 0,
    SCREEN_SETUP,
    SCREEN_HOME,
    SCREEN_SENSORS,
    SCREEN_ALARMS,
    SCREEN_VISION,
    SCREEN_AI,        // NEW: AI Insights screen
    SCREEN_REMOTE,
    SCREEN_SETTINGS,
    SCREEN_COUNT
} ScreenID_t;

// ============ UI State ============
typedef struct {
    ScreenID_t currentScreen;
    bool sidebarCollapsed;
    bool systemRunning;
    bool setupCompleted;
    uint8_t unackedAlarms;
} UIState_t;

// ============ Public Functions ============
#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_show_main(void);  // Call after splash to show main UI
void ui_navigate_to(ScreenID_t screen);
void ui_refresh(void);
UIState_t* ui_get_state(void);
void ui_toggle_sidebar(void);
void ui_toggle_system(void);
void ui_update_sensors(void);

#ifdef __cplusplus
}
#endif

#endif // UI_MANAGER_H
