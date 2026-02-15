# SIGNALTAP - Industrial IoT Retrofit Solution

ESP32-P4 based industrial monitoring system with 7" MIPI-DSI touchscreen display.

## Features

- **4 Demo Profiles**: CNC Machine Shop, Cold Storage Chiller, Compressed Air System, Custom PLC Setup
- **AI Predictive Maintenance**: Health scoring, anomaly detection, failure prediction, predictive insights
- **QR Code Dashboard**: Scan to access web dashboard for remote monitoring and OTA updates
- **OTA Firmware Updates**: Push firmware updates via mobile/web dashboard
- **Live Simulation**: Sensor values, vision elements, AI predictions update in real-time
- **6 Navigation Screens**: Home, Sensors, Alarms, Vision, AI Agent, Settings
- **Vision Monitoring**: Stack lights, LED arrays, 7-segment displays, I/O panels
- **Industrial UI**: Dark theme with cyan/purple accents, professional HMI appearance

## Hardware

- **MCU**: ESP32-P4
- **Display**: JD9165 MIPI-DSI 7" 1024x600
- **Touch**: GT911 capacitive touch controller
- **Board**: JC1060P470C

## AI Features

### Health Scoring
- Real-time machine health score (0-100%)
- Color-coded arc indicator (green/yellow/red)
- Based on sensor patterns and anomaly detection

### Predictive Insights
Each demo profile includes 3 AI predictions with:
- Severity level (Normal/Warning/Critical)
- Confidence percentage
- Time-to-event prediction
- Actionable descriptions

### Remote Dashboard
- QR code links to device-specific web dashboard
- Mobile-friendly interface
- Real-time sensor data streaming
- OTA update management

### OTA Updates
- Current/available firmware display
- One-tap update initiation
- Progress bar during updates
- Rollback capability

## Setup Instructions

### 1. Install LVGL Library
- Arduino Library Manager → Search "lvgl"
- Install version **9.2.2** (NOT 9.4.0 - has gradient bugs)

### 2. Configure lv_conf.h
Copy `lv_conf.h` from this project to your Arduino libraries folder:
```
C:\Users\<username>\Documents\Arduino\libraries\lv_conf.h
```
**Important**: Place it NEXT TO (not inside) the lvgl folder.

### 3. Board Configuration
In Arduino IDE:
- Board: ESP32P4 Dev Module
- PSRAM: OPI PSRAM
- Partition Scheme: app3M_fat9M_16MB

### 4. Upload
Connect via USB and upload the sketch.

## Project Structure

```
signaltap_arduino/
├── signaltap_arduino.ino    # Main sketch with simulation
├── config.h                  # Configuration defines
├── pins_config.h             # Display/touch pin config
├── lv_conf.h                 # LVGL configuration
├── lvgl_port_v9.c/h          # LVGL display port
├── lvgl_sw_rotation.c        # Display initialization
└── src/
    ├── ui/
    │   ├── ui_manager.cpp/h  # Complete UI implementation
    │   ├── ui_theme.h        # Color definitions
    │   └── logo.c            # Splash screen logo
    ├── data/
    │   └── demo_profiles.h   # 4 demo configs + AI states
    ├── lcd/
    │   └── esp_lcd_jd9165.*  # JD9165 MIPI-DSI driver
    └── touch/
        └── esp_lcd_touch_gt911.* # GT911 touch driver
```

## Demo Profiles

### CNC Machine Shop
- **Sensors**: Spindle Load, Coolant Flow, Spindle Speed
- **Vision**: Part counter, stack light, 8 status LEDs
- **AI**: Tool life prediction, bearing wear detection, coolant optimization

### Cold Storage Chiller
- **Sensors**: Compressor Power, Supply/Return Temp
- **Vision**: 7-segment error code display
- **AI**: Compressor efficiency monitoring, discharge pressure trends, defrost anomalies

### Compressed Air System
- **Sensors**: Tank Pressure, Oil Temperature, Motor Current
- **Vision**: Pressure gauge, LOAD/IDLE state indicator
- **AI**: Oil quality analysis, air filter status, energy optimization

### Custom PLC Setup
- **Sensors**: Chamber Temp/Pressure, Compressor Pressure
- **Vision**: 8-bit DI/DQ panels, analog output display
- **AI**: Process drift detection, cycle time analysis, I/O pattern learning

## Controls

- **Demo Selector**: Tap header button to cycle through profiles
- **Navigation**: Use sidebar buttons to switch screens
- **AI Agent**: View predictions, scan QR for remote access
- **Power Button**: Toggle simulation running/stopped

## Troubleshooting

### Compilation Errors
- Ensure LVGL 9.2.2 is installed (not 9.4.0)
- Verify lv_conf.h is in the correct location
- Check that PSRAM is enabled in board settings

### Display Issues
- Portrait mode? Check pins_config.h rotation settings
- Wrong resolution? Verify LCD_H_RES=1024, LCD_V_RES=600
