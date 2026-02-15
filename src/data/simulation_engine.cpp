// SIGNALTAP Simulation Engine Implementation
// Physics-correlated, scenario-driven simulation for all 4 demo profiles
#include "simulation_engine.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

static SimEngine_t engine;

// ============ Helper: Smooth approach to target ============
static float approach(float current, float target, float rate) {
    float diff = target - current;
    return current + diff * rate;
}

// ============ Helper: Clamp value ============
static float clampf(float val, float lo, float hi) {
    if (val < lo) return lo;
    if (val > hi) return hi;
    return val;
}

// ============ Helper: Add noise ============
static float noise(float amplitude) {
    return (random(-1000, 1001) / 1000.0f) * amplitude;
}

// ============ Helper: Push to ring buffer ============
static void history_push(SensorHistory_t* h, float value) {
    h->buffer[h->head] = value;
    h->head = (h->head + 1) % SENSOR_HISTORY_LEN;
    if (h->count < SENSOR_HISTORY_LEN) h->count++;
}

// ============ Helper: Format current time string ============
static void format_time(char* buf, size_t len) {
    unsigned long s = millis() / 1000;
    uint8_t h = (s / 3600) % 24;
    uint8_t m = (s / 60) % 60;
    uint8_t sec = s % 60;
    snprintf(buf, len, "%02d:%02d:%02d", h, m, sec);
}

// ============ Helper: Add dynamic alarm ============
static void add_alarm(SimState_t* sim, const char* severity, const char* message) {
    // Don't duplicate - check if same message already active
    for (int i = 0; i < sim->dynamicAlarmCount; i++) {
        if (sim->dynamicAlarms[i].active && strcmp(sim->dynamicAlarms[i].message, message) == 0) {
            return;  // Already exists
        }
    }

    // Find empty slot or use oldest
    int slot = -1;
    for (int i = 0; i < MAX_DYNAMIC_ALARMS; i++) {
        if (!sim->dynamicAlarms[i].active) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        // Overwrite oldest
        slot = 0;
        unsigned long oldest = sim->dynamicAlarms[0].triggerTime;
        for (int i = 1; i < MAX_DYNAMIC_ALARMS; i++) {
            if (sim->dynamicAlarms[i].triggerTime < oldest) {
                oldest = sim->dynamicAlarms[i].triggerTime;
                slot = i;
            }
        }
    }

    DynamicAlarm_t* a = &sim->dynamicAlarms[slot];
    strncpy(a->severity, severity, sizeof(a->severity) - 1);
    a->severity[sizeof(a->severity) - 1] = '\0';
    strncpy(a->message, message, sizeof(a->message) - 1);
    a->message[sizeof(a->message) - 1] = '\0';
    format_time(a->time, sizeof(a->time));
    a->acked = false;
    a->active = true;
    a->triggerTime = millis();

    if (slot >= sim->dynamicAlarmCount) {
        sim->dynamicAlarmCount = slot + 1;
    }
}

// ============ Helper: Clear non-acked fault alarms on recovery ============
static void clear_fault_alarms(SimState_t* sim) {
    for (int i = 0; i < sim->dynamicAlarmCount; i++) {
        DynamicAlarm_t* a = &sim->dynamicAlarms[i];
        if (a->active && !a->acked && strcmp(a->severity, "error") == 0) {
            a->active = false;
        }
    }
}

// ============ State Transition ============
static uint16_t get_state_duration(ScenarioState_t state) {
    switch (state) {
        case SCENARIO_NORMAL:       return SCENARIO_NORMAL_DURATION_S;
        case SCENARIO_DEGRADATION:  return SCENARIO_DEGRADATION_DURATION_S;
        case SCENARIO_WARNING:      return SCENARIO_WARNING_DURATION_S;
        case SCENARIO_FAULT:        return SCENARIO_FAULT_DURATION_S;
        case SCENARIO_RECOVERY:     return SCENARIO_RECOVERY_DURATION_S;
        default:                    return SCENARIO_NORMAL_DURATION_S;
    }
}

static ScenarioState_t next_state(ScenarioState_t current) {
    switch (current) {
        case SCENARIO_NORMAL:       return SCENARIO_DEGRADATION;
        case SCENARIO_DEGRADATION:  return SCENARIO_WARNING;
        case SCENARIO_WARNING:      return SCENARIO_FAULT;
        case SCENARIO_FAULT:        return SCENARIO_RECOVERY;
        case SCENARIO_RECOVERY:     return SCENARIO_NORMAL;
        default:                    return SCENARIO_NORMAL;
    }
}

static void transition_state(SimState_t* sim) {
    sim->scenarioState = next_state(sim->scenarioState);
    sim->stateEnteredAt = millis();
    sim->stateTimer = 0;

    if (sim->scenarioState == SCENARIO_NORMAL) {
        sim->cycleCount++;
    }

    if (sim->scenarioState == SCENARIO_RECOVERY) {
        clear_fault_alarms(sim);
    }
}

// ================================================================
// Per-Demo Physics Models
// Each demo has correlated sensor behavior based on scenario state
// ================================================================

// ============ CNC Machine Shop Physics ============
static void update_cnc(DemoProfile_t* demo, SimState_t* sim) {
    float progress = sim->stateTimer / (float)get_state_duration(sim->scenarioState);
    Vision_t* v = &demo->vision;

    switch (sim->scenarioState) {
        case SCENARIO_NORMAL:
            // Stable operation: moderate load, good coolant, mid-speed
            sim->sensorTargets[0] = 55.0f + noise(3.0f);   // Spindle load ~55%
            sim->sensorTargets[1] = 13.0f + noise(0.5f);   // Coolant flow ~13 L/min
            sim->sensorTargets[2] = 4500.0f + noise(100);  // Spindle speed ~4500 RPM
            sim->targetHealthScore = 90;
            sim->targetFailureProb = 5.0f;
            v->stackLight = "green";
            v->leds.run = true; v->leds.ready = true;
            v->leds.error = false; v->leds.fault = false;
            // Occasional part increment
            if (random(100) < 8) v->partCount++;
            break;

        case SCENARIO_DEGRADATION:
            // Load rising, coolant flow dropping (filter clogging)
            sim->sensorTargets[0] = 55.0f + progress * 25.0f + noise(2.0f);  // 55→80%
            sim->sensorTargets[1] = 13.0f - progress * 4.0f + noise(0.3f);   // 13→9 L/min
            sim->sensorTargets[2] = 4500.0f - progress * 500.0f + noise(80); // Slight RPM drop
            sim->targetHealthScore = 90 - (int)(progress * 15);  // 90→75
            sim->targetFailureProb = 5.0f + progress * 15.0f;
            if (progress > 0.5f) v->stackLight = "yellow";
            if (progress > 0.3f) {
                add_alarm(sim, "info", "Spindle load trending upward");
            }
            if (progress > 0.7f) {
                add_alarm(sim, "warning", "Coolant flow below optimal range");
                v->leds.coolant = false;  // Coolant LED goes off
            }
            if (random(100) < 5) v->partCount++;
            break;

        case SCENARIO_WARNING:
            // High load, low coolant, bearings heating up
            sim->sensorTargets[0] = 82.0f + progress * 8.0f + noise(2.0f);  // 82→90%
            sim->sensorTargets[1] = 8.5f - progress * 2.0f + noise(0.3f);   // 8.5→6.5
            sim->sensorTargets[2] = 3800.0f - progress * 400.0f + noise(60);
            sim->targetHealthScore = 75 - (int)(progress * 15);  // 75→60
            sim->targetFailureProb = 20.0f + progress * 20.0f;
            v->stackLight = "yellow";
            add_alarm(sim, "warning", "Spindle load above 80% threshold");
            if (progress > 0.6f) {
                add_alarm(sim, "error", "Coolant level critically low");
            }
            v->leds.coolant = false;
            if (random(100) < 3) v->partCount++;
            break;

        case SCENARIO_FAULT:
            // Overload trip, spindle stops
            sim->sensorTargets[0] = 95.0f + noise(3.0f);    // Pegged high
            sim->sensorTargets[1] = 4.0f + noise(0.5f);      // Minimal flow
            sim->sensorTargets[2] = 1000.0f * (1.0f - progress) + noise(50);  // Spinning down
            sim->targetHealthScore = 55 - (int)(progress * 15);  // 55→40
            sim->targetFailureProb = 65.0f + progress * 25.0f;
            v->stackLight = "red";
            v->leds.error = true; v->leds.fault = true;
            v->leds.run = false; v->leds.spindle = false;
            add_alarm(sim, "error", "FAULT: Spindle overload protection tripped");
            // Update AI insights to reflect the fault
            demo->ai.insights[0].severity = INSIGHT_CRITICAL;
            demo->ai.insights[0].title = "Spindle Bearing Overload";
            demo->ai.insights[0].description = "Bearing overload detected - immediate inspection required";
            demo->ai.insights[0].timeframe = "immediate";
            break;

        case SCENARIO_RECOVERY:
            // Coolant restored, load dropping, spindle restarting
            sim->sensorTargets[0] = 90.0f - progress * 35.0f + noise(2.0f);  // 90→55%
            sim->sensorTargets[1] = 5.0f + progress * 8.0f + noise(0.3f);    // 5→13
            sim->sensorTargets[2] = 500.0f + progress * 4000.0f + noise(100); // Spooling up
            sim->targetHealthScore = 45 + (int)(progress * 45);  // 45→90
            sim->targetFailureProb = 80.0f - progress * 75.0f;
            if (progress < 0.3f) v->stackLight = "yellow";
            else v->stackLight = "green";
            if (progress > 0.5f) {
                v->leds.run = true; v->leds.spindle = true;
                v->leds.error = false; v->leds.fault = false;
                v->leds.coolant = true;
            }
            add_alarm(sim, "info", "System recovery in progress");
            // Restore AI insights
            demo->ai.insights[0].severity = INSIGHT_WARNING;
            demo->ai.insights[0].title = "Spindle Bearing Wear";
            demo->ai.insights[0].description = "Vibration pattern suggests bearing replacement in ~14 days";
            demo->ai.insights[0].timeframe = "14 days";
            break;
    }
}

// ============ Cold Storage Chiller Physics ============
static void update_chiller(DemoProfile_t* demo, SimState_t* sim) {
    float progress = sim->stateTimer / (float)get_state_duration(sim->scenarioState);
    Vision_t* v = &demo->vision;

    switch (sim->scenarioState) {
        case SCENARIO_NORMAL:
            // Good delta-T, moderate power, stable temps
            sim->sensorTargets[0] = 26.0f + noise(1.0f);     // Compressor power ~26 kW
            sim->sensorTargets[1] = 2.0f + noise(0.3f);      // Supply temp ~2°C
            sim->sensorTargets[2] = 7.5f + noise(0.3f);      // Return temp ~7.5°C
            sim->targetHealthScore = 88;
            sim->targetFailureProb = 5.0f;
            v->errorCode = "---";
            // Update KPI delta-T
            demo->kpis[0].value = "5.5";
            demo->kpis[3].value = "OK";
            demo->kpis[3].good = true;
            break;

        case SCENARIO_DEGRADATION:
            // Compressor working harder, supply temp rising, refrigerant low
            sim->sensorTargets[0] = 26.0f + progress * 10.0f + noise(0.8f);   // 26→36 kW
            sim->sensorTargets[1] = 2.0f + progress * 3.0f + noise(0.2f);     // 2→5°C
            sim->sensorTargets[2] = 7.5f + progress * 2.0f + noise(0.2f);     // 7.5→9.5°C
            sim->targetHealthScore = 88 - (int)(progress * 18);  // 88→70
            sim->targetFailureProb = 5.0f + progress * 18.0f;
            if (progress > 0.4f) {
                add_alarm(sim, "warning", "Supply temperature rising above setpoint");
                demo->kpis[0].value = "4.0";
            }
            if (progress > 0.7f) {
                add_alarm(sim, "warning", "Compressor power consumption elevated");
            }
            break;

        case SCENARIO_WARNING:
            // High discharge pressure, poor delta-T, compressor cycling
            sim->sensorTargets[0] = 38.0f + progress * 8.0f + noise(1.5f);   // Cycling spikes
            sim->sensorTargets[1] = 5.5f + progress * 3.0f + noise(0.4f);    // Supply drifting
            sim->sensorTargets[2] = 10.0f + progress * 3.0f + noise(0.3f);   // Return high
            sim->targetHealthScore = 70 - (int)(progress * 15);
            sim->targetFailureProb = 25.0f + progress * 20.0f;
            add_alarm(sim, "warning", "High discharge pressure detected");
            demo->kpis[0].value = "3.0";
            demo->kpis[3].value = "WARN";
            demo->kpis[3].good = false;
            if (progress > 0.5f) v->errorCode = "E07";
            break;

        case SCENARIO_FAULT:
            // Compressor tripped, temps rising fast
            sim->sensorTargets[0] = 8.0f + noise(2.0f);                       // Compressor off/cycling
            sim->sensorTargets[1] = 9.0f + progress * 6.0f + noise(0.5f);     // Supply warming fast
            sim->sensorTargets[2] = 14.0f + progress * 5.0f + noise(0.4f);    // Return warming
            sim->targetHealthScore = 50 - (int)(progress * 20);
            sim->targetFailureProb = 55.0f + progress * 35.0f;
            v->errorCode = "E07";
            demo->kpis[3].value = "FAULT";
            demo->kpis[3].good = false;
            add_alarm(sim, "error", "E07: High discharge pressure - compressor tripped");
            demo->ai.insights[0].severity = INSIGHT_CRITICAL;
            demo->ai.insights[0].title = "Compressor Trip";
            demo->ai.insights[0].description = "High discharge pressure caused safety cutout";
            demo->ai.insights[0].timeframe = "immediate";
            break;

        case SCENARIO_RECOVERY:
            // Compressor restarting, temps slowly dropping
            sim->sensorTargets[0] = 12.0f + progress * 16.0f + noise(1.0f);   // Power ramping
            sim->sensorTargets[1] = 14.0f - progress * 12.0f + noise(0.3f);   // Supply cooling
            sim->sensorTargets[2] = 18.0f - progress * 10.5f + noise(0.3f);   // Return cooling
            sim->targetHealthScore = 35 + (int)(progress * 53);
            sim->targetFailureProb = 80.0f - progress * 75.0f;
            if (progress > 0.3f) v->errorCode = "---";
            if (progress > 0.6f) {
                demo->kpis[3].value = "OK";
                demo->kpis[3].good = true;
                demo->kpis[0].value = "5.0";
            }
            add_alarm(sim, "info", "Chiller recovery - compressor restarting");
            demo->ai.insights[0].severity = INSIGHT_WARNING;
            demo->ai.insights[0].title = "Compressor Efficiency Drop";
            demo->ai.insights[0].description = "Power consumption 15% above baseline - check refrigerant levels";
            demo->ai.insights[0].timeframe = "immediate";
            break;
    }
}

// ============ Compressed Air System Physics ============
static void update_compressor(DemoProfile_t* demo, SimState_t* sim) {
    float progress = sim->stateTimer / (float)get_state_duration(sim->scenarioState);
    Vision_t* v = &demo->vision;

    switch (sim->scenarioState) {
        case SCENARIO_NORMAL:
            // Good pressure, normal oil temp, moderate current
            sim->sensorTargets[0] = 8.0f + noise(0.3f);      // Tank pressure ~8 bar
            sim->sensorTargets[1] = 75.0f + noise(2.0f);     // Oil temp ~75°C
            sim->sensorTargets[2] = 32.0f + noise(1.5f);     // Motor current ~32A
            sim->targetHealthScore = 94;
            sim->targetFailureProb = 3.0f;
            v->pressure = sim->sensorTargets[0];
            v->oilTemp = sim->sensorTargets[1];
            v->state = "LOAD";
            demo->kpis[3].value = "LOAD";
            demo->kpis[3].good = true;
            // Periodic LOAD/IDLE cycling
            if (sim->stateTimer % 8 < 2) {
                v->state = "IDLE";
                demo->kpis[3].value = "IDLE";
            }
            break;

        case SCENARIO_DEGRADATION:
            // Oil temp rising (filter degradation), pressure dropping (leak starting)
            sim->sensorTargets[0] = 8.0f - progress * 1.5f + noise(0.2f);     // 8→6.5 bar
            sim->sensorTargets[1] = 75.0f + progress * 18.0f + noise(1.5f);   // 75→93°C
            sim->sensorTargets[2] = 32.0f + progress * 8.0f + noise(1.0f);    // 32→40A (working harder)
            sim->targetHealthScore = 94 - (int)(progress * 14);
            sim->targetFailureProb = 3.0f + progress * 12.0f;
            v->pressure = sim->sensorTargets[0];
            v->oilTemp = sim->sensorTargets[1];
            v->state = "LOAD";
            if (progress > 0.5f) {
                add_alarm(sim, "warning", "Oil temperature trending above normal");
            }
            if (progress > 0.7f) {
                add_alarm(sim, "info", "Tank pressure below optimal - check for leaks");
            }
            break;

        case SCENARIO_WARNING:
            // High oil temp, low pressure, motor straining
            sim->sensorTargets[0] = 6.2f - progress * 1.2f + noise(0.3f);    // 6.2→5 bar
            sim->sensorTargets[1] = 95.0f + progress * 15.0f + noise(2.0f);  // 95→110°C!
            sim->sensorTargets[2] = 42.0f + progress * 10.0f + noise(1.5f);  // 42→52A
            sim->targetHealthScore = 80 - (int)(progress * 20);
            sim->targetFailureProb = 18.0f + progress * 25.0f;
            v->pressure = sim->sensorTargets[0];
            v->oilTemp = sim->sensorTargets[1];
            add_alarm(sim, "warning", "Oil temp approaching high limit (100C)");
            if (progress > 0.5f) {
                add_alarm(sim, "error", "Tank pressure critically low");
            }
            demo->kpis[3].value = "LOAD";
            break;

        case SCENARIO_FAULT:
            // Thermal shutdown, motor trips
            sim->sensorTargets[0] = 4.5f - progress * 2.5f + noise(0.2f);   // Pressure bleeding off
            sim->sensorTargets[1] = 112.0f + noise(1.0f);                    // Oil overtemp
            sim->sensorTargets[2] = 5.0f * (1.0f - progress) + noise(0.5f); // Motor stopping
            sim->targetHealthScore = 55 - (int)(progress * 25);
            sim->targetFailureProb = 50.0f + progress * 40.0f;
            v->pressure = sim->sensorTargets[0];
            v->oilTemp = sim->sensorTargets[1];
            v->state = "FAULT";
            demo->kpis[3].value = "FAULT";
            demo->kpis[3].good = false;
            add_alarm(sim, "error", "THERMAL SHUTDOWN: Oil temperature exceeded limit");
            demo->ai.insights[0].severity = INSIGHT_CRITICAL;
            demo->ai.insights[0].title = "Thermal Shutdown";
            demo->ai.insights[0].description = "Oil overtemperature caused compressor safety shutdown";
            demo->ai.insights[0].timeframe = "immediate";
            break;

        case SCENARIO_RECOVERY:
            // Cooling down, motor restarting, pressure building
            sim->sensorTargets[0] = 2.5f + progress * 5.5f + noise(0.2f);     // 2.5→8 bar
            sim->sensorTargets[1] = 110.0f - progress * 35.0f + noise(1.0f);  // 110→75°C
            sim->sensorTargets[2] = 5.0f + progress * 27.0f + noise(1.0f);    // 5→32A
            sim->targetHealthScore = 35 + (int)(progress * 59);
            sim->targetFailureProb = 85.0f - progress * 82.0f;
            v->pressure = sim->sensorTargets[0];
            v->oilTemp = sim->sensorTargets[1];
            if (progress > 0.4f) {
                v->state = "LOAD";
                demo->kpis[3].value = "LOAD";
                demo->kpis[3].good = true;
            } else {
                v->state = "IDLE";
                demo->kpis[3].value = "IDLE";
            }
            add_alarm(sim, "info", "Compressor cooling down - restart in progress");
            demo->ai.insights[0].severity = INSIGHT_WARNING;
            demo->ai.insights[0].title = "Oil Quality Good";
            demo->ai.insights[0].description = "Viscosity and contamination levels within spec";
            demo->ai.insights[0].timeframe = "stable";
            break;
    }
}

// ============ Custom PLC Physics ============
static void update_plc(DemoProfile_t* demo, SimState_t* sim) {
    float progress = sim->stateTimer / (float)get_state_duration(sim->scenarioState);
    Vision_t* v = &demo->vision;

    switch (sim->scenarioState) {
        case SCENARIO_NORMAL:
            // Stable chamber, process running
            sim->sensorTargets[0] = 85.0f + noise(1.5f);     // Chamber temp ~85°C
            sim->sensorTargets[1] = 500.0f + noise(15.0f);   // Chamber press ~500 mbar
            sim->sensorTargets[2] = 5.0f + noise(0.2f);      // Compressor ~5 bar
            sim->targetHealthScore = 92;
            sim->targetFailureProb = 4.0f;
            // Normal I/O pattern: alternating cycle
            if (sim->stateTimer % 6 < 3) {
                v->diA[0] = true; v->diA[1] = false; v->diA[2] = true;
                v->dqA[0] = true; v->dqA[3] = true;
            } else {
                v->diA[0] = false; v->diA[1] = true; v->diA[2] = false;
                v->dqA[0] = false; v->dqA[3] = false;
            }
            v->aq0 = 65 + (int)noise(3);
            demo->kpis[3].value = "AUTO";
            demo->kpis[3].good = true;
            break;

        case SCENARIO_DEGRADATION:
            // Chamber temp drifting up, process slowing
            sim->sensorTargets[0] = 85.0f + progress * 30.0f + noise(2.0f);   // 85→115°C
            sim->sensorTargets[1] = 500.0f + progress * 150.0f + noise(10.0f); // 500→650 mbar
            sim->sensorTargets[2] = 5.0f - progress * 0.8f + noise(0.15f);     // Slight pressure drop
            sim->targetHealthScore = 92 - (int)(progress * 15);
            sim->targetFailureProb = 4.0f + progress * 12.0f;
            v->aq0 = 65 + (int)(progress * 20);  // Output ramping up (compensating)
            if (progress > 0.5f) {
                add_alarm(sim, "warning", "Chamber temperature drifting above setpoint");
                v->diA[5] = true;  // Warning DI activates
            }
            break;

        case SCENARIO_WARNING:
            // Overtemp, pressure spike, cycle times extending
            sim->sensorTargets[0] = 120.0f + progress * 40.0f + noise(3.0f);    // 120→160°C
            sim->sensorTargets[1] = 660.0f + progress * 200.0f + noise(20.0f);  // Pressure rising
            sim->sensorTargets[2] = 4.0f - progress * 1.0f + noise(0.2f);
            sim->targetHealthScore = 75 - (int)(progress * 15);
            sim->targetFailureProb = 18.0f + progress * 25.0f;
            v->aq0 = 90 + (int)(progress * 10);  // Maxing out
            if (v->aq0 > 100) v->aq0 = 100;
            v->diA[5] = true; v->diA[6] = true;  // Warning + alarm DIs
            add_alarm(sim, "warning", "Chamber temp approaching safety limit");
            if (progress > 0.7f) {
                add_alarm(sim, "error", "Pressure spike detected - process deviation");
            }
            break;

        case SCENARIO_FAULT:
            // Safety shutdown, all outputs off
            sim->sensorTargets[0] = 165.0f + noise(2.0f);    // Overtemp
            sim->sensorTargets[1] = 850.0f + noise(30.0f);   // High pressure
            sim->sensorTargets[2] = 2.0f + noise(0.3f);      // Low air
            sim->targetHealthScore = 50 - (int)(progress * 20);
            sim->targetFailureProb = 55.0f + progress * 35.0f;
            // Safety shutdown - all DQs off
            for (int i = 0; i < 8; i++) v->dqA[i] = false;
            v->diA[6] = true; v->diA[7] = true;  // Fault + E-stop DIs
            v->aq0 = 0;
            demo->kpis[3].value = "STOP";
            demo->kpis[3].good = false;
            add_alarm(sim, "error", "SAFETY SHUTDOWN: Chamber overtemperature");
            demo->ai.insights[0].severity = INSIGHT_CRITICAL;
            demo->ai.insights[0].title = "Process Safety Shutdown";
            demo->ai.insights[0].description = "Chamber overtemperature triggered emergency stop";
            demo->ai.insights[0].timeframe = "immediate";
            break;

        case SCENARIO_RECOVERY:
            // Cooling, restarting process
            sim->sensorTargets[0] = 160.0f - progress * 75.0f + noise(2.0f);    // 160→85°C
            sim->sensorTargets[1] = 850.0f - progress * 350.0f + noise(15.0f);  // 850→500
            sim->sensorTargets[2] = 2.5f + progress * 2.5f + noise(0.15f);      // Pressure building
            sim->targetHealthScore = 35 + (int)(progress * 57);
            sim->targetFailureProb = 80.0f - progress * 76.0f;
            // Gradually restore I/O
            if (progress > 0.4f) {
                v->dqA[0] = true; v->dqA[3] = true;
                v->diA[6] = false; v->diA[7] = false;
                v->aq0 = (int)(progress * 65);
            }
            if (progress > 0.7f) {
                demo->kpis[3].value = "AUTO";
                demo->kpis[3].good = true;
                v->diA[5] = false;
            }
            add_alarm(sim, "info", "Process restarting - chamber cooling");
            demo->ai.insights[0].severity = INSIGHT_WARNING;
            demo->ai.insights[0].title = "Process Drift Detected";
            demo->ai.insights[0].description = "Chamber temperature variance increased 12% this week";
            demo->ai.insights[0].timeframe = "monitoring";
            break;
    }
}

// ================================================================
// Main Update Loop
// ================================================================
void sim_init(void) {
    memset(&engine, 0, sizeof(engine));

    for (int d = 0; d < DEMO_COUNT; d++) {
        SimState_t* sim = &engine.demos[d];
        sim->scenarioState = SCENARIO_NORMAL;
        sim->stateEnteredAt = millis();
        sim->stateTimer = 0;
        sim->cycleCount = 0;
        sim->dynamicAlarmCount = 0;
        sim->otaInProgress = false;
        sim->otaProgress = 0;

        // Initialize targets from current profile values
        DemoProfile_t* demo = &demoProfiles[d];
        for (int i = 0; i < 3; i++) {
            sim->sensorTargets[i] = demo->sensors[i].value;
            sim->history[i].head = 0;
            sim->history[i].count = 0;
        }
        sim->targetHealthScore = demo->ai.healthScore;
        sim->targetFailureProb = demo->ai.failureProbability;
    }

    engine.lastUpdateMs = millis();
    engine.initialized = true;
}

void sim_update(void) {
    if (!engine.initialized) return;

    uint8_t demoIdx = getDemoIndex();
    SimState_t* sim = &engine.demos[demoIdx];
    DemoProfile_t* demo = getDemo();

    // Increment state timer
    sim->stateTimer++;

    // Check for state transition
    if (sim->stateTimer >= get_state_duration(sim->scenarioState)) {
        transition_state(sim);
    }

    // Run per-demo physics model
    switch (demoIdx) {
        case 0: update_cnc(demo, sim); break;
        case 1: update_chiller(demo, sim); break;
        case 2: update_compressor(demo, sim); break;
        case 3: update_plc(demo, sim); break;
    }

    // Smooth sensor values toward targets
    for (int i = 0; i < 3; i++) {
        float target = clampf(sim->sensorTargets[i], demo->sensors[i].min, demo->sensors[i].max);
        demo->sensors[i].value = approach(demo->sensors[i].value, target, 0.15f);
        demo->sensors[i].value = clampf(demo->sensors[i].value, demo->sensors[i].min, demo->sensors[i].max);

        // Record history
        history_push(&sim->history[i], demo->sensors[i].value);
    }

    // Smooth AI values
    demo->ai.healthScore = (uint8_t)approach((float)demo->ai.healthScore,
                                              (float)sim->targetHealthScore, 0.12f);
    demo->ai.failureProbability = approach(demo->ai.failureProbability,
                                           sim->targetFailureProb, 0.1f);

    // Anomaly count tied to scenario
    switch (sim->scenarioState) {
        case SCENARIO_NORMAL:   demo->ai.anomalyCount = 0; break;
        case SCENARIO_DEGRADATION: demo->ai.anomalyCount = 1; break;
        case SCENARIO_WARNING:  demo->ai.anomalyCount = 2; break;
        case SCENARIO_FAULT:    demo->ai.anomalyCount = 3 + random(0, 2); break;
        case SCENARIO_RECOVERY: demo->ai.anomalyCount = 1; break;
    }

    // Data points always incrementing
    demo->ai.dataPoints += random(10, 40);

    // Update insight confidence based on scenario
    for (int i = 0; i < 3; i++) {
        int baseConf;
        if (sim->scenarioState == SCENARIO_FAULT) {
            baseConf = 88 + random(0, 10);
        } else if (sim->scenarioState == SCENARIO_WARNING) {
            baseConf = 75 + random(0, 15);
        } else {
            baseConf = 55 + random(0, 25);
        }
        demo->ai.insights[i].confidence = (uint8_t)clampf((float)baseConf, 40.0f, 99.0f);
    }

    // OTA simulation
    if (sim->otaInProgress) {
        sim->otaProgress += 2 + random(0, 3);
        if (sim->otaProgress >= 100) {
            sim->otaProgress = 100;
            sim->otaInProgress = false;
            add_alarm(sim, "info", "Firmware update completed successfully (v1.1.0)");
        }
    }

    engine.lastUpdateMs = millis();
}

// ============ Getters ============

ScenarioState_t sim_get_scenario(void) {
    if (!engine.initialized) return SCENARIO_NORMAL;
    return engine.demos[getDemoIndex()].scenarioState;
}

const char* sim_get_scenario_name(void) {
    switch (sim_get_scenario()) {
        case SCENARIO_NORMAL:       return "Normal";
        case SCENARIO_DEGRADATION:  return "Degrading";
        case SCENARIO_WARNING:      return "Warning";
        case SCENARIO_FAULT:        return "FAULT";
        case SCENARIO_RECOVERY:     return "Recovering";
        default:                    return "Unknown";
    }
}

SensorHistory_t* sim_get_history(uint8_t sensorIndex) {
    if (!engine.initialized || sensorIndex >= 3) return NULL;
    return &engine.demos[getDemoIndex()].history[sensorIndex];
}

uint8_t sim_get_alarm_count(void) {
    if (!engine.initialized) return 0;
    SimState_t* sim = &engine.demos[getDemoIndex()];
    uint8_t count = 0;
    for (int i = 0; i < sim->dynamicAlarmCount; i++) {
        if (sim->dynamicAlarms[i].active) count++;
    }
    return count;
}

DynamicAlarm_t* sim_get_alarm(uint8_t index) {
    if (!engine.initialized) return NULL;
    SimState_t* sim = &engine.demos[getDemoIndex()];
    uint8_t count = 0;
    for (int i = 0; i < sim->dynamicAlarmCount; i++) {
        if (sim->dynamicAlarms[i].active) {
            if (count == index) return &sim->dynamicAlarms[i];
            count++;
        }
    }
    return NULL;
}

void sim_ack_alarm(uint8_t index) {
    DynamicAlarm_t* a = sim_get_alarm(index);
    if (a) a->acked = true;
}

void sim_start_ota(void) {
    if (!engine.initialized) return;
    SimState_t* sim = &engine.demos[getDemoIndex()];
    sim->otaInProgress = true;
    sim->otaProgress = 0;
    add_alarm(sim, "info", "Firmware update started - downloading v1.1.0");
}

bool sim_ota_active(void) {
    if (!engine.initialized) return false;
    return engine.demos[getDemoIndex()].otaInProgress;
}

uint8_t sim_ota_progress(void) {
    if (!engine.initialized) return 0;
    return engine.demos[getDemoIndex()].otaProgress;
}

SimState_t* sim_get_state(void) {
    if (!engine.initialized) return NULL;
    return &engine.demos[getDemoIndex()];
}
