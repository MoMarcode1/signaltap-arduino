// SIGNALTAP Configuration
#ifndef CONFIG_H
#define CONFIG_H

// ============ Feature Flags ============
#define ENABLE_WIFI         0   // WiFi disabled - conflicts with MIPI-DSI
#define ENABLE_ETHERNET     0
#define ENABLE_MQTT         0
#define ENABLE_DEMO_MODE    1   // Enable demo profiles
#define ENABLE_ONBOARDING   1   // Show one-time setup page before main screens

// Remote dashboard URL used by QR codes (ESP Remote View + AI screen)
// Update this when you publish index.html (for example, GitHub Pages URL).
#define REMOTE_DASHBOARD_URL "https://YOUR_GITHUB_USERNAME.github.io/signaltap_arduino/"

// ============ Display Configuration ============
#define DISPLAY_WIDTH       1024
#define DISPLAY_HEIGHT      600
#define DISPLAY_BRIGHTNESS  100

// ============ UI Layout ============
#define SIDEBAR_WIDTH       160
#define SIDEBAR_COLLAPSED   60
#define HEADER_HEIGHT       48

// ============ Timing ============
#define SPLASH_DURATION_MS  2500
#define SENSOR_UPDATE_MS    1000
#define LVGL_TICK_MS        5

#endif // CONFIG_H
