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
#include "touch.h"
#include "settings.h"
#include "ui_manager.h"

// Global objects
Display display;
Accelerometer accel;
SignalProcessor processor;
BleService bleService;
TouchManager touchMgr;
UIManager uiMgr;
Settings settings;

// Hardware timer for precise sampling
hw_timer_t* sampleTimer = nullptr;
volatile bool sampleFlag = false;

// Timer ISR - just sets flag, actual I2C read happens in loop
void IRAM_ATTR onSampleTimer() {
    sampleFlag = true;
}

// Timing variables
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
    delay(500);  // Give serial more time to initialize

    Serial.println();
    Serial.println("================================");
    Serial.println("  gSENSOR - High-G Accelerometer");
    Serial.println("================================");
    Serial.println();
    Serial.println("[Setup] Starting...");

    // Initialize display first (shows splash screen)
    Serial.println("[Setup] Initializing display...");
    if (!display.begin()) {
        Serial.println("ERROR: Display initialization failed!");
        while (1) {
            delay(500);
        }
    }
    Serial.println("[Setup] Display OK, showing splash...");

    display.showSplash();
    Serial.println("[Setup] Splash shown, waiting...");
    delay(1500);  // Show splash for 1.5 seconds

    // Initialize accelerometer
    Serial.println("[Setup] Initializing ADXL375...");
    sensorOk = accel.begin();

    if (!sensorOk) {
        display.showError("ADXL375 NOT FOUND");
        Serial.println("ERROR: Accelerometer initialization failed!");
        Serial.println("Check wiring:");
        Serial.println("  JST GND  -> ADXL375 GND");
        Serial.println("  JST 3.3V -> ADXL375 VIN");
        Serial.println("  JST TX   -> ADXL375 SDA");
        Serial.println("  JST RX   -> ADXL375 SCL");
        // Continue anyway to show error on display
    } else {
        Serial.println("[Setup] ADXL375 OK, drawing UI...");
        // Clear display and draw static UI
        display.clear();
        display.drawStaticUI();
        Serial.println("[Setup] UI drawn");
    }

    // Initialize touch controller
    Serial.println("[Setup] Initializing touch...");
    if (touchMgr.begin()) {
        Serial.println("[Setup] Touch controller OK");
    } else {
        Serial.println("WARNING: Touch controller not found");
    }

    // Initialize BLE
    Serial.println("[Setup] Initializing BLE...");
    bleService.begin();
    Serial.println("[Setup] BLE OK");

    // Register BLE command callback
    bleService.setCommandCallback([](uint8_t cmd) {
        switch (cmd) {
            case BLE_CMD_RESET_PEAK:
                processor.resetPeak();
                display.resetGaugeMax();
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

    // Initialize physical button
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    Serial.println("[Setup] Button initialized on GPIO9");

    // Set up hardware timer for precise 100Hz sampling
    // Timer 0, prescaler 80 (80MHz/80 = 1MHz tick rate), count up
    sampleTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(sampleTimer, &onSampleTimer, true);
    // Alarm every 10000 microseconds (10ms = 100Hz), auto-reload enabled
    timerAlarmWrite(sampleTimer, ADXL_SAMPLE_INTERVAL_MS * 1000, true);
    timerAlarmEnable(sampleTimer);
    Serial.printf("[Setup] Sample timer configured for %d Hz\n", ADXL_SAMPLE_RATE_HZ);

    // Initialize timing
    lastDisplayTime = millis();
    lastBLENotifyTime = millis();

    Serial.println("[Setup] Complete! Entering main loop...");
}

/**
 * @brief Arduino main loop
 */
void loop() {
    // Sample accelerometer FIRST when timer fires (highest priority!)
    // The timer ISR sets sampleFlag at precise 100Hz intervals
    if (sampleFlag && sensorOk) {
        sampleFlag = false;  // Clear flag immediately

        AccelData raw;
        if (accel.read(raw)) {
            // Process through moving average filter
            AccelData filtered = processor.process(raw);

            // Serial output in CSV format for plotting (if enabled)
            // Format: timestamp,x,y,z,magnitude,peak
            if (DEBUG_ENABLED && settings.serialEnabled) {
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

    uint32_t now = millis();

    // Handle physical button (active LOW, debounced)
    static bool lastButtonState = HIGH;
    static uint32_t lastButtonTime = 0;
    bool buttonState = digitalRead(PIN_BUTTON);

    if (buttonState != lastButtonState && (now - lastButtonTime) > 200) {
        lastButtonTime = now;
        if (buttonState == LOW) {
            // Button pressed - toggle screen
            if (uiMgr.getScreen() == UIScreen::MAIN_GAUGE) {
                uiMgr.setScreen(UIScreen::SETTINGS);
                Serial.println("[Button] Opening settings");
            } else {
                uiMgr.setScreen(UIScreen::MAIN_GAUGE);
                Serial.println("[Button] Back to gauge");
            }
        }
        lastButtonState = buttonState;
    }

    // Handle touch input (skipped if no touch controller)
    touchMgr.update();
    TouchEvent event = touchMgr.getEvent();
    if (event.gesture != TouchGesture::NONE) {
        uiMgr.handleTouch(event, settings);

        // Handle BLE enable/disable from settings
        bleService.setEnabled(settings.bleEnabled);
    }

    // Handle peak reset from UI (long press)
    if (uiMgr.peakResetRequested()) {
        processor.resetPeak();
        display.resetGaugeMax();
        if (DEBUG_ENABLED && settings.serialEnabled) {
            Serial.println("Peak reset (touch)");
        }
    }

    // Update display at lower rate (to prevent flicker and save CPU)
    if (now - lastDisplayTime >= DISPLAY_UPDATE_INTERVAL_MS) {
        lastDisplayTime = now;

        // Check if screen changed (need to prepare)
        if (uiMgr.screenChanged()) {
            display.prepareScreen();
        }

        // Draw appropriate screen
        if (uiMgr.getScreen() == UIScreen::MAIN_GAUGE) {
            AccelData accelData = {0, 0, 0};
            float magnitude = 0.0f;
            float peak = 0.0f;

            if (sensorOk) {
                accelData = processor.getLastFiltered();
                magnitude = processor.getFilteredMagnitude();
                peak = processor.getPeakMagnitude();
            }

            display.update(accelData, magnitude, peak);
        } else {
            // Settings screen
            display.drawSettingsScreen(settings, bleService.isConnected());
        }
    }

    // Send BLE notifications at configured rate (if BLE is enabled)
    if (settings.bleEnabled && bleService.isConnected() && sensorOk) {
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
                display.resetGaugeMax();
                if (DEBUG_ENABLED && settings.serialEnabled) {
                    Serial.println("Peak reset");
                }
                break;

            case 'c':
            case 'C':
                processor.reset();
                if (DEBUG_ENABLED && settings.serialEnabled) {
                    Serial.println("Filters reset");
                }
                break;

            default:
                break;
        }
    }
}
