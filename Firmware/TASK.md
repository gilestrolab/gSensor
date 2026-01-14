# gSENSOR Firmware - Task Tracking

## Completed Tasks

### 2026-01-13 - Initial Firmware Development
- [x] Create PlatformIO project structure
- [x] Configure display (switched from TFT_eSPI to LovyanGFX)
- [x] Define hardware pin configuration
- [x] Implement moving average filter
- [x] Implement ADXL375 driver with custom I2C pins
- [x] Implement display module with arc gauge
- [x] Create main application loop
- [x] Fix I2C pins (GPIO20 for SCL, not GPIO22)
- [x] Add sensor calibration offsets
- [x] Create project documentation

## Current Tasks

- [x] Create serial plotter tool

## Backlog

### Software Improvements
- [ ] Add auto-calibration routine on startup
- [ ] Add data logging to SPIFFS
- [ ] Implement WiFi streaming mode
- [ ] Add configurable sample rate via serial command
- [ ] Reduce serial debug output verbosity (add quiet mode)

### UI Enhancements
- [ ] Add min/max hold indicators
- [ ] Implement waveform view mode
- [ ] Add settings screen (touch-based)
- [ ] Improve gauge aesthetics

## Discovered During Work

- ESP32-C3 only has GPIO0-21 (no GPIO22!)
- TFT_eSPI doesn't work reliably with ESP32-2424S012; use LovyanGFX instead
- ADXL375 has significant per-unit zero-g offsets requiring calibration
- Backlight must be enabled AFTER display init (per manufacturer's code)

## Notes

### Wiring Reference
```
ESP32-2424S012 JST     Adafruit ADXL375
-------------------    -----------------
GND  ───────────────── GND
3.3V ───────────────── VIN
TX (GPIO21) ────────── SDA
RX (GPIO20) ────────── SCL
```

Note: ESP32-C3 only has GPIO0-21, there is no GPIO22!

### Calibration Values
Measured with sensor flat, screen facing up:
```cpp
OFFSET_X = 0.35f   // X reads +0.35g, should be 0
OFFSET_Y = -0.60f  // Y reads -0.6g, should be 0
OFFSET_Z = 0.70f   // Z reads +1.7g, should be +1.0g
```

### Serial Commands
- `r` or `R` - Reset peak value
- `c` or `C` - Reset filters (calibrate)

### Debug Output Format
```
RAW: X=0.00 Y=0.00 Z=1.00 | Mag=1.00 | Filtered=1.00 | Peak=1.00
```
