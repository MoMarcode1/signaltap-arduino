// SIGNALTAP Demo Profiles Data
#ifndef DEMO_PROFILES_H
#define DEMO_PROFILES_H

#include <Arduino.h>

// ============ Vision Type Enum ============
typedef enum {
    VISION_CNC = 0,
    VISION_CHILLER,
    VISION_COMPRESSOR,
    VISION_CUSTOM
} VisionType_t;

// ============ AI Insight Types ============
typedef enum {
    INSIGHT_NORMAL = 0,
    INSIGHT_WARNING,
    INSIGHT_CRITICAL
} InsightSeverity_t;

typedef struct {
    const char* title;
    const char* description;
    InsightSeverity_t severity;
    uint8_t confidence;  // 0-100%
    const char* timeframe;  // "2 hours", "3 days", etc.
} AIInsight_t;

typedef struct {
    uint8_t healthScore;        // 0-100 overall machine health
    uint8_t anomalyCount;       // Current anomalies detected
    float failureProbability;   // 0-100% chance of failure in next 24h
    const char* nextMaintenance; // Predicted maintenance date
    const char* modelStatus;    // "Learning", "Ready", "Updating"
    uint32_t dataPoints;        // Training data points collected
    AIInsight_t insights[3];    // Top 3 predictions
} AIState_t;

// ============ Data Structures ============
typedef struct {
    const char* name;
    const char* unit;
    uint32_t color;
    float min;
    float max;
    uint8_t decimals;
    const char* type;
    float value;
} Sensor_t;

typedef struct {
    const char* label;
    const char* value;
    const char* unit;
    bool good;
} KPI_t;

typedef struct {
    const char* severity;  // "error", "warning", "info"
    const char* message;
    const char* time;
    bool acked;
} Alarm_t;

// CNC LED states
typedef struct {
    bool run;
    bool feed;
    bool spindle;
    bool coolant;
    bool program;
    bool error;
    bool fault;
    bool ready;
} CNCLeds_t;

// Unified Vision structure with all possible fields
typedef struct {
    VisionType_t type;
    
    // CNC fields
    uint16_t partCount;
    const char* stackLight;  // "red", "yellow", "green"
    CNCLeds_t leds;
    
    // Chiller fields
    const char* errorCode;
    
    // Compressor fields
    float pressure;
    float oilTemp;
    const char* state;  // "LOAD", "IDLE"
    
    // Custom PLC fields
    bool diA[8];   // Digital inputs .0-.7
    bool dqA[8];   // Digital outputs .0-.7
    uint8_t aq0;   // Analog output %
} Vision_t;

// Demo Profile structure
typedef struct {
    const char* name;
    const char* sub;
    uint32_t color;
    
    Sensor_t sensors[3];
    KPI_t kpis[4];
    Alarm_t alarms[3];
    Vision_t vision;
    AIState_t ai;  // NEW: AI predictive maintenance state
} DemoProfile_t;

// ============ Demo Profiles ============
#define DEMO_COUNT 4

static DemoProfile_t demoProfiles[DEMO_COUNT] = {
    // ========== CNC Machine Shop ==========
    {
        .name = "CNC Machine Shop",
        .sub = "Siemens 810D Controller",
        .color = 0x3b82f6,
        .sensors = {
            {"Spindle Load", "%", 0x3b82f6, 0, 100, 1, "CT Coil", 67.2f},
            {"Coolant Flow", "L/min", 0x06b6d4, 0, 20, 1, "Flow Sensor", 12.5f},
            {"Spindle Speed", "RPM", 0x10b981, 0, 8000, 0, "Encoder", 4200.0f}
        },
        .kpis = {
            {"OEE", "87.3", "%", false},
            {"Cycles", "142", "", false},
            {"Target", "160", "", false},
            {"Status", "RUN", "", true}
        },
        .alarms = {
            {"warning", "Spindle load above 80%", "08:15:22", false},
            {"info", "Part count: 142 completed", "08:10:00", true},
            {"error", "Coolant level low", "07:45:30", false}
        },
        .vision = {
            .type = VISION_CNC,
            .partCount = 142,
            .stackLight = "green",
            .leds = {true, false, true, true, true, false, false, true},
            .errorCode = "",
            .pressure = 0,
            .oilTemp = 0,
            .state = "",
            .diA = {false},
            .dqA = {false},
            .aq0 = 0
        },
        .ai = {
            .healthScore = 87,
            .anomalyCount = 1,
            .failureProbability = 12.5f,
            .nextMaintenance = "Feb 18",
            .modelStatus = "Ready",
            .dataPoints = 48250,
            .insights = {
                {"Spindle Bearing Wear", "Vibration pattern suggests bearing replacement in ~14 days", INSIGHT_WARNING, 78, "14 days"},
                {"Coolant Efficiency", "Flow rate optimization could improve by 8%", INSIGHT_NORMAL, 65, "ongoing"},
                {"Tool Life Prediction", "Current tool approaching end of life cycle", INSIGHT_WARNING, 82, "~50 parts"}
            }
        }
    },
    
    // ========== Cold Storage Chiller ==========
    {
        .name = "Cold Storage Chiller",
        .sub = "Carrier 30RB Unit",
        .color = 0x06b6d4,
        .sensors = {
            {"Compressor Power", "kW", 0xf59e0b, 0, 50, 1, "CT 3-Phase", 28.5f},
            {"Supply Temp", "\xC2\xB0""C", 0x06b6d4, -10, 20, 1, "4-20mA RTD", 2.3f},
            {"Return Temp", "\xC2\xB0""C", 0x8b5cf6, -5, 25, 1, "4-20mA RTD", 8.1f}
        },
        .kpis = {
            {"\xCE\x94""T", "5.8", "\xC2\xB0""C", false},
            {"Runtime", "1847", "hrs", false},
            {"Energy", "892", "kWh", false},
            {"Status", "OK", "", true}
        },
        .alarms = {
            {"error", "E07: High discharge pressure", "06:30:15", false},
            {"warning", "Compressor cycling high", "06:15:00", false},
            {"info", "Runtime: 1800 hours", "05:00:00", true}
        },
        .vision = {
            .type = VISION_CHILLER,
            .partCount = 0,
            .stackLight = "",
            .leds = {false},
            .errorCode = "---",
            .pressure = 0,
            .oilTemp = 0,
            .state = "",
            .diA = {false},
            .dqA = {false},
            .aq0 = 0
        },
        .ai = {
            .healthScore = 72,
            .anomalyCount = 2,
            .failureProbability = 28.3f,
            .nextMaintenance = "Feb 15",
            .modelStatus = "Ready",
            .dataPoints = 125840,
            .insights = {
                {"Compressor Efficiency Drop", "Power consumption 15% above baseline - check refrigerant levels", INSIGHT_CRITICAL, 91, "immediate"},
                {"Discharge Pressure Trend", "Gradual increase detected over 72 hours", INSIGHT_WARNING, 76, "3 days"},
                {"Defrost Cycle Anomaly", "Irregular defrost timing pattern detected", INSIGHT_NORMAL, 62, "monitoring"}
            }
        }
    },
    
    // ========== Compressed Air System ==========
    {
        .name = "Compressed Air",
        .sub = "Atlas Copco GA30",
        .color = 0x10b981,
        .sensors = {
            {"Tank Pressure", "bar", 0x10b981, 0, 12, 1, "Analog Gauge", 8.2f},
            {"Oil Temperature", "\xC2\xB0""C", 0xef4444, 20, 120, 0, "Analog Gauge", 78.0f},
            {"Motor Current", "A", 0xf59e0b, 0, 60, 1, "CT Coil", 34.2f}
        },
        .kpis = {
            {"Load", "72", "%", false},
            {"Cost/Day", "48", "\xE2\x82\xAC", false},
            {"Service", "342", "hrs", false},
            {"State", "LOAD", "", true}
        },
        .alarms = {
            {"warning", "Oil temp approaching limit", "09:20:45", false},
            {"info", "Service due in 342 hrs", "09:00:00", true},
            {"", "", "", true}  // Empty slot
        },
        .vision = {
            .type = VISION_COMPRESSOR,
            .partCount = 0,
            .stackLight = "",
            .leds = {false},
            .errorCode = "",
            .pressure = 8.2f,
            .oilTemp = 78.0f,
            .state = "LOAD",
            .diA = {false},
            .dqA = {false},
            .aq0 = 0
        },
        .ai = {
            .healthScore = 94,
            .anomalyCount = 0,
            .failureProbability = 3.2f,
            .nextMaintenance = "Mar 05",
            .modelStatus = "Ready",
            .dataPoints = 89420,
            .insights = {
                {"Oil Quality Good", "Viscosity and contamination levels within spec", INSIGHT_NORMAL, 95, "stable"},
                {"Air Filter Status", "Pressure drop suggests filter change in ~2 weeks", INSIGHT_WARNING, 71, "2 weeks"},
                {"Energy Optimization", "Load/unload cycle could be optimized for 5% savings", INSIGHT_NORMAL, 68, "ongoing"}
            }
        }
    },
    
    // ========== Custom PLC Setup ==========
    {
        .name = "Custom PLC Setup",
        .sub = "Siemens S7-1200",
        .color = 0x8b5cf6,
        .sensors = {
            {"Chamber Temp", "\xC2\xB0""C", 0x22d3ee, 20, 200, 1, "RTD PT100", 85.3f},
            {"Chamber Press", "mbar", 0xf59e0b, 0, 1013, 0, "4-20mA", 485.0f},
            {"Compressor", "Bar", 0x10b981, 0, 10, 2, "0-10V", 4.72f}
        },
        .kpis = {
            {"Efficiency", "94.2", "%", false},
            {"Uptime", "99.1", "%", false},
            {"Cycles", "8472", "", false},
            {"Mode", "AUTO", "", true}
        },
        .alarms = {
            {"warning", "Chamber temp approaching limit", "07:32:15", false},
            {"info", "Maintenance in 5 days", "07:30:00", true},
            {"error", "Pressure spike detected", "07:28:45", false}
        },
        .vision = {
            .type = VISION_CUSTOM,
            .partCount = 0,
            .stackLight = "",
            .leds = {false},
            .errorCode = "",
            .pressure = 0,
            .oilTemp = 0,
            .state = "",
            .diA = {true, false, true, true, false, false, true, false},
            .dqA = {true, false, false, true, false, true, false, false},
            .aq0 = 65
        },
        .ai = {
            .healthScore = 91,
            .anomalyCount = 1,
            .failureProbability = 8.7f,
            .nextMaintenance = "Feb 22",
            .modelStatus = "Learning",
            .dataPoints = 12850,
            .insights = {
                {"Process Drift Detected", "Chamber temperature variance increased 12% this week", INSIGHT_WARNING, 74, "monitoring"},
                {"Cycle Time Analysis", "Recent cycles 3% slower than baseline average", INSIGHT_NORMAL, 58, "ongoing"},
                {"I/O Pattern Learning", "Model collecting baseline patterns - 78% complete", INSIGHT_NORMAL, 78, "2 days"}
            }
        }
    }
};

// Current demo state
static uint8_t currentDemoIndex = 0;

static inline void setDemo(uint8_t index) {
    if (index < DEMO_COUNT) {
        currentDemoIndex = index;
    }
}

static inline DemoProfile_t* getDemo() {
    return &demoProfiles[currentDemoIndex];
}

static inline void nextDemo() {
    currentDemoIndex = (currentDemoIndex + 1) % DEMO_COUNT;
}

static inline uint8_t getDemoIndex() {
    return currentDemoIndex;
}

#endif // DEMO_PROFILES_H
