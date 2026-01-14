/**
 * @file config.h
 * @brief Hardware configuration and constants for gSENSOR
 *
 * Pin definitions for ESP32-2424S012 board with ADXL375 accelerometer.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ==================== Board Information ====================
// ESP32-2424S012 uses ESP32-C3-MINI-1U module
// - 4MB Flash
// - USB-C for programming and power
// - 240x240 GC9A01 round display
// - CST816D touch controller (unused in this project)

// ==================== Display Pins (SPI - Fixed) ====================
// These are defined in User_Setup.h for TFT_eSPI
// Listed here for reference only
constexpr uint8_t PIN_TFT_MOSI = 7;
constexpr uint8_t PIN_TFT_SCLK = 6;
constexpr uint8_t PIN_TFT_CS   = 10;
constexpr uint8_t PIN_TFT_DC   = 2;
constexpr uint8_t PIN_TFT_BL   = 3;

// ==================== ADXL375 I2C Pins (JST Connector) ====================
// The JST connector exposes UART pins which we repurpose for I2C
// JST Pinout: GND, 3.3V, TX (GPIO21), RX (GPIO20)
// Note: ESP32-C3 only has GPIO0-21, there is no GPIO22!
constexpr uint8_t PIN_ADXL_SDA = 21;  // JST TX pin -> I2C SDA
constexpr uint8_t PIN_ADXL_SCL = 20;  // JST RX pin -> I2C SCL

// ADXL375 I2C address (ALT ADDRESS pin LOW on Adafruit board)
constexpr uint8_t ADXL375_I2C_ADDR = 0x53;

// Calibration offsets (measured with sensor flat, screen up)
// These values are subtracted from raw readings
// Adjust these based on your sensor's readings at rest
constexpr float OFFSET_X = 0.35f;   // X reads +0.35g when should be 0
constexpr float OFFSET_Y = -0.60f;  // Y reads -0.6g when should be 0
constexpr float OFFSET_Z = 0.70f;   // Z reads +1.7g when should be +1.0g (gravity)

// ==================== Display Configuration ====================
constexpr uint16_t DISPLAY_WIDTH  = 240;
constexpr uint16_t DISPLAY_HEIGHT = 240;
constexpr uint16_t DISPLAY_CENTER_X = DISPLAY_WIDTH / 2;
constexpr uint16_t DISPLAY_CENTER_Y = DISPLAY_HEIGHT / 2;

// Display refresh rate (milliseconds)
constexpr uint32_t DISPLAY_UPDATE_INTERVAL_MS = 50;  // 20 Hz refresh

// ==================== ADXL375 Configuration ====================
// Fixed range: +/- 200g
// Sensitivity: 49 mg/LSB (0.049 g per LSB)
constexpr float ADXL375_SCALE_FACTOR = 0.049f;  // g per LSB
constexpr float ADXL375_MAX_G = 200.0f;

// Sample rate (Hz) - ADXL375 can output up to 3200 Hz
// We'll use a reasonable rate for display purposes
constexpr uint32_t ADXL_SAMPLE_RATE_HZ = 100;
constexpr uint32_t ADXL_SAMPLE_INTERVAL_MS = 1000 / ADXL_SAMPLE_RATE_HZ;

// ==================== Signal Processing ====================
// Moving average filter window size
// Larger = smoother but more latency
// At 100 Hz sample rate:
//   10 samples = 100ms averaging window
//   20 samples = 200ms averaging window
constexpr size_t MOVING_AVG_WINDOW_SIZE = 10;

// ==================== UI Configuration ====================
// Colors (RGB565 format)
constexpr uint16_t COLOR_BACKGROUND = 0x0000;   // Black
constexpr uint16_t COLOR_TEXT       = 0xFFFF;   // White
constexpr uint16_t COLOR_LOW_G      = 0x07E0;   // Green (0-10g)
constexpr uint16_t COLOR_MED_G      = 0xFFE0;   // Yellow (10-50g)
constexpr uint16_t COLOR_HIGH_G     = 0xFD20;   // Orange (50-100g)
constexpr uint16_t COLOR_EXTREME_G  = 0xF800;   // Red (>100g)

// G-force thresholds for color changes
constexpr float G_THRESHOLD_LOW     = 10.0f;
constexpr float G_THRESHOLD_MED     = 50.0f;
constexpr float G_THRESHOLD_HIGH    = 100.0f;

// Arc gauge configuration
constexpr uint16_t GAUGE_RADIUS_OUTER = 110;
constexpr uint16_t GAUGE_RADIUS_INNER = 85;
constexpr float GAUGE_START_ANGLE = 135.0f;   // Start at bottom-left
constexpr float GAUGE_END_ANGLE   = 405.0f;   // End at bottom-right (270 degree sweep)
constexpr float GAUGE_MAX_VALUE   = 200.0f;   // Full scale = 200g

// ==================== Serial Debug ====================
constexpr uint32_t SERIAL_BAUD_RATE = 115200;

// Set to true to enable debug output via serial
constexpr bool DEBUG_ENABLED = true;

// ==================== BLE Configuration ====================
// Device name visible during BLE scanning
constexpr const char* BLE_DEVICE_NAME = "gSENSOR";

// Custom service and characteristic UUIDs
// Base UUID: 12345678-1234-5678-1234-56789abcdef0
constexpr const char* BLE_SERVICE_UUID        = "12345678-1234-5678-1234-56789abcde00";
constexpr const char* BLE_CHAR_ACCEL_UUID     = "12345678-1234-5678-1234-56789abcde01";
constexpr const char* BLE_CHAR_PEAK_UUID      = "12345678-1234-5678-1234-56789abcde02";
constexpr const char* BLE_CHAR_CONTROL_UUID   = "12345678-1234-5678-1234-56789abcde03";
constexpr const char* BLE_CHAR_CONFIG_UUID    = "12345678-1234-5678-1234-56789abcde04";

// BLE notification rate (Hz) - lower saves power
constexpr uint8_t BLE_DEFAULT_NOTIFY_RATE_HZ = 20;
constexpr uint8_t BLE_MAX_NOTIFY_RATE_HZ = 50;
constexpr uint8_t BLE_MIN_NOTIFY_RATE_HZ = 5;
constexpr uint32_t BLE_NOTIFY_INTERVAL_MS = 1000 / BLE_DEFAULT_NOTIFY_RATE_HZ;

// Control commands (received via BLE_CHAR_CONTROL)
constexpr uint8_t BLE_CMD_RESET_PEAK    = 0x01;
constexpr uint8_t BLE_CMD_RESET_FILTERS = 0x02;

#endif // CONFIG_H
