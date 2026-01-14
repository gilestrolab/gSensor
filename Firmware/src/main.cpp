/**
 * @file main.cpp
 * @brief gSENSOR main application
 *
 * Real-time g-force display using ESP32-2424S012 board with ADXL375 accelerometer.
 *
 * Hardware:
 *   - ESP32-C3 based ESP32-2424S012 board
 *   - GC9A01 240x240 round LCD display
 *   - ADXL375 high-g accelerometer (±200g) via JST connector
 *
 * Wiring (JST connector to ADXL375):
 *   - GND  -> GND
 *   - 3.3V -> VIN
 *   - TX (GPIO21) -> SDA
 *   - RX (GPIO22) -> SCL
 */

#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "accelerometer.h"
#include "signal_processing.h"
#include "ble_service.h"

// Global objects
Display display;
Accelerometer accel;
SignalProcessor processor;
BleService bleService;

// Timing variables
uint32_t lastSampleTime = 0;
uint32_t lastDisplayTime = 0;
uint32_t lastBLENotifyTime = 0;

// State
bool sensorOk = false;

/**
 * @brief Arduino setup function
 */
void setup() {
    // Initialize serial for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);  // Give serial time to initialize

    if (DEBUG_ENABLED) {
        Serial.println();
        Serial.println("================================");
        Serial.println("  gSENSOR - High-G Accelerometer");
        Serial.println("================================");
        Serial.println();
    }

    // Initialize display first (shows splash screen)
    if (!display.begin()) {
        if (DEBUG_ENABLED) {
            Serial.println("ERROR: Display initialization failed!");
        }
        // Can't show error on display, so just hang with LED blink if available
        while (1) {
            delay(500);
        }
    }

    display.showSplash();
    delay(1500);  // Show splash for 1.5 seconds

    // Initialize accelerometer
    if (DEBUG_ENABLED) {
        Serial.println("Initializing ADXL375...");
    }

    sensorOk = accel.begin();

    if (!sensorOk) {
        display.showError("ADXL375 NOT FOUND");
        if (DEBUG_ENABLED) {
            Serial.println("ERROR: Accelerometer initialization failed!");
            Serial.println("Check wiring:");
            Serial.println("  JST GND  -> ADXL375 GND");
            Serial.println("  JST 3.3V -> ADXL375 VIN");
            Serial.println("  JST TX   -> ADXL375 SDA");
            Serial.println("  JST RX   -> ADXL375 SCL");
        }
        // Continue anyway to show error on display
    } else {
        // Clear display and draw static UI
        display.clear();
        display.drawStaticUI();

        if (DEBUG_ENABLED) {
            Serial.println("System ready!");
            Serial.println();
            Serial.println("Output format: timestamp,x,y,z,magnitude,peak");
        }
    }

    // Initialize BLE
    bleService.begin();

    // Register BLE command callback
    bleService.setCommandCallback([](uint8_t cmd) {
        switch (cmd) {
            case BLE_CMD_RESET_PEAK:
                processor.resetPeak();
                if (DEBUG_ENABLED) {
                    Serial.println("BLE: Peak reset");
                }
                break;

            case BLE_CMD_RESET_FILTERS:
                processor.reset();
                if (DEBUG_ENABLED) {
                    Serial.println("BLE: Filters reset");
                }
                break;

            default:
                if (DEBUG_ENABLED) {
                    Serial.print("BLE: Unknown command 0x");
                    Serial.println(cmd, HEX);
                }
                break;
        }
    });

    // Initialize timing
    lastSampleTime = millis();
    lastDisplayTime = millis();
    lastBLENotifyTime = millis();
}

/**
 * @brief Arduino main loop
 */
void loop() {
    uint32_t now = millis();

    // Sample accelerometer at configured rate
    if (sensorOk && (now - lastSampleTime >= ADXL_SAMPLE_INTERVAL_MS)) {
        lastSampleTime = now;

        AccelData raw;
        if (accel.read(raw)) {
            // Process through moving average filter
            AccelData filtered = processor.process(raw);

            // Serial output in CSV format for plotting
            // Format: timestamp,x,y,z,magnitude,peak
            if (DEBUG_ENABLED) {
                float mag = processor.getFilteredMagnitude();
                float peak = processor.getPeakMagnitude();

                Serial.print(millis());
                Serial.print(",");
                Serial.print(filtered.x, 3);
                Serial.print(",");
                Serial.print(filtered.y, 3);
                Serial.print(",");
                Serial.print(filtered.z, 3);
                Serial.print(",");
                Serial.print(mag, 3);
                Serial.print(",");
                Serial.println(peak, 3);
            }
        }
    }

    // Update display at lower rate (to prevent flicker and save CPU)
    if (now - lastDisplayTime >= DISPLAY_UPDATE_INTERVAL_MS) {
        lastDisplayTime = now;

        if (sensorOk) {
            const AccelData& filtered = processor.getLastFiltered();
            float magnitude = processor.getFilteredMagnitude();
            float peak = processor.getPeakMagnitude();

            display.update(filtered, magnitude, peak);
        }
    }

    // Send BLE notifications at configured rate
    if (bleService.isConnected() && sensorOk) {
        uint32_t bleInterval = 1000 / bleService.getNotificationRate();
        if (now - lastBLENotifyTime >= bleInterval) {
            lastBLENotifyTime = now;

            const AccelData& filtered = processor.getLastFiltered();
            float magnitude = processor.getFilteredMagnitude();
            float peak = processor.getPeakMagnitude();

            bleService.notifyAccelData(now, filtered, magnitude);

            // Also send peak update periodically (every ~500ms)
            static uint32_t lastPeakNotify = 0;
            if (now - lastPeakNotify >= 500) {
                lastPeakNotify = now;
                bleService.notifyPeak(now, peak);
            }
        }
    }

    // Small delay to prevent tight loop (saves power)
    delay(1);
}

/**
 * @brief Handle serial commands (optional feature)
 *
 * Commands:
 *   'r' - Reset peak value
 *   'c' - Calibrate (reset filters)
 */
void serialEvent() {
    while (Serial.available()) {
        char cmd = Serial.read();

        switch (cmd) {
            case 'r':
            case 'R':
                processor.resetPeak();
                if (DEBUG_ENABLED) {
                    Serial.println("Peak reset");
                }
                break;

            case 'c':
            case 'C':
                processor.reset();
                if (DEBUG_ENABLED) {
                    Serial.println("Filters reset");
                }
                break;

            default:
                break;
        }
    }
}
