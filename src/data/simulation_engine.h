// SIGNALTAP Simulation Engine
// Scenario-driven, physics-correlated sensor simulation with dynamic alarms & AI
#ifndef SIMULATION_ENGINE_H
#define SIMULATION_ENGINE_H

#include <Arduino.h>
#include "demo_profiles.h"

// ============ Scenario States ============
typedef enum {
    SCENARIO_NORMAL = 0,    // Everything healthy, stable operation
    SCENARIO_DEGRADATION,   // Gradual drift, early warning signs
    SCENARIO_WARNING,       // Thresholds being approached/crossed
    SCENARIO_FAULT,         // Active fault condition
    SCENARIO_RECOVERY       // Returning to normal after fault
} ScenarioState_t;

// ============ Scenario Timing ============
#define SCENARIO_NORMAL_DURATION_S      45  // 45s normal operation
#define SCENARIO_DEGRADATION_DURATION_S 20  // 20s gradual degradation
#define SCENARIO_WARNING_DURATION_S     15  // 15s warning state
#define SCENARIO_FAULT_DURATION_S       12  // 12s fault condition
#define SCENARIO_RECOVERY_DURATION_S    10  // 10s recovery back to normal

// ============ Sensor History ============
#define SENSOR_HISTORY_LEN 60  // 60 data points (~1 min at 1Hz)

typedef struct {
    float buffer[SENSOR_HISTORY_LEN];
    uint8_t head;
    uint8_t count;
} SensorHistory_t;

// ============ Dynamic Alarm System ============
#define MAX_DYNAMIC_ALARMS 8

typedef struct {
    char severity[8];    // "error", "warning", "info"
    char message[80];
    char time[12];
    bool acked;
    bool active;         // Currently active (auto-clears on recovery)
    unsigned long triggerTime;
} DynamicAlarm_t;

// ============ Simulation State (per demo) ============
typedef struct {
    ScenarioState_t scenarioState;
    unsigned long stateEnteredAt;    // millis() when state was entered
    unsigned long stateTimer;        // Seconds in current state

    // Sensor targets (physics model drives these, actual values smooth toward them)
    float sensorTargets[3];

    // Sensor history ring buffers
    SensorHistory_t history[3];

    // Dynamic alarms
    DynamicAlarm_t dynamicAlarms[MAX_DYNAMIC_ALARMS];
    uint8_t dynamicAlarmCount;

    // AI state targets
    uint8_t targetHealthScore;
    float targetFailureProb;

    // Scenario cycle counter (for variety)
    uint8_t cycleCount;

    // OTA simulation
    bool otaInProgress;
    uint8_t otaProgress;  // 0-100
} SimState_t;

// ============ Engine State ============
typedef struct {
    SimState_t demos[DEMO_COUNT];
    unsigned long lastUpdateMs;
    bool initialized;
} SimEngine_t;

// ============ Public API ============

// Initialize the simulation engine
void sim_init(void);

// Call every SENSOR_UPDATE_MS (1 second) - drives the whole simulation
void sim_update(void);

// Get current scenario state for active demo
ScenarioState_t sim_get_scenario(void);
const char* sim_get_scenario_name(void);

// Get sensor history for sparkline rendering
SensorHistory_t* sim_get_history(uint8_t sensorIndex);

// Get dynamic alarms (merged with static profile alarms)
uint8_t sim_get_alarm_count(void);
DynamicAlarm_t* sim_get_alarm(uint8_t index);
void sim_ack_alarm(uint8_t index);

// OTA simulation
void sim_start_ota(void);
bool sim_ota_active(void);
uint8_t sim_ota_progress(void);

// Get the sim state for the current demo
SimState_t* sim_get_state(void);

#endif // SIMULATION_ENGINE_H
