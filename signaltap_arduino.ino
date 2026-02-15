/*
 * SIGNALTAP - Industrial IoT Retrofit Solution
 * ESP32-P4 with JD9165 MIPI-DSI Display + GT911 Touch
 *
 * Hardware: JC1060P470C 7" 1024x600 Display
 * Framework: Arduino + LVGL v9.x
 *
 * Simulation Engine drives physics-correlated sensors through
 * scenario cycles: Normal → Degradation → Warning → Fault → Recovery
 */

#include <Arduino.h>
#include <lvgl.h>
#include "config.h"
#include "src/ui/ui_manager.h"
#include "src/data/demo_profiles.h"
#include "src/data/simulation_engine.h"
#include "lvgl_port_v9.h"

// External function from lvgl_sw_rotation.c
extern "C" void lvgl_sw_rotation_main(void);

// Timing
static unsigned long lastSensorUpdate = 0;
static unsigned long startTime = 0;
static bool splashDone = false;

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("========================================");
    Serial.println("SIGNALTAP - Industrial IoT Retrofit");
    Serial.println("ESP32-P4 + MIPI-DSI Display");
    Serial.println("Scenario Simulation Engine v2.0");
    Serial.println("========================================");

    // Initialize display, touch, and LVGL
    lvgl_sw_rotation_main();

    // Initialize simulation engine
    sim_init();

    startTime = millis();

    Serial.println("Display initialized");
    Serial.println("Simulation engine initialized");
    Serial.println("UI initialized - showing splash screen");
}

void loop() {
    // NOTE: lvgl_port handles lv_timer_handler() internally via FreeRTOS task
    // We only need to handle our application logic here

    unsigned long now = millis();

    // Handle splash screen transition (after SPLASH_DURATION_MS)
    if (!splashDone && (now - startTime >= SPLASH_DURATION_MS)) {
        splashDone = true;
        Serial.println("Splash done - navigating to home");

        if (lvgl_port_lock(100)) {
            ui_show_main();
            lvgl_port_unlock();
        }
    }

    // Update simulation every second (only after splash)
    if (splashDone && (now - lastSensorUpdate >= SENSOR_UPDATE_MS)) {
        lastSensorUpdate = now;

        UIState_t* state = ui_get_state();
        if (state && state->systemRunning) {
            // Run the simulation engine (handles all physics, alarms, AI)
            sim_update();

            // Refresh display with lock
            if (lvgl_port_lock(100)) {
                ui_refresh();
                lvgl_port_unlock();
            }
        }
    }

    delay(10);  // Small delay to yield to other tasks
}
