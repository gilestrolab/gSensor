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

// Current sample rate (can be changed at runtime via serial command)
uint32_t currentSampleRateHz = ADXL_DEFAULT_SAMPLE_RATE_HZ;

// Timer ISR - just sets flag, actual I2C read happens in loop
void IRAM_ATTR onSampleTimer() {
    sampleFlag = true;
}

/**
 * @brief Change the accelerometer sample rate at runtime
 *
 * Updates both the hardware timer interval and the ADXL375 data rate register.
 * Valid rates: 100, 200, 400, 800 Hz (limited by serial bandwidth)
 *
 * @param rateHz Target sample rate in Hz
 * @return true if rate was changed successfully
 */
bool setSampleRate(uint32_t rateHz) {
    // Map Hz to ADXL data rate enum
    adxl3xx_dataRate_t adxlRate;
    switch (rateHz) {
        case 100:  adxlRate = ADXL3XX_DATARATE_100_HZ; break;
        case 200:  adxlRate = ADXL3XX_DATARATE_200_HZ; break;
        case 400:  adxlRate = ADXL3XX_DATARATE_400_HZ; break;
        case 800:  adxlRate = ADXL3XX_DATARATE_800_HZ; break;
        default:
            Serial.printf("Invalid rate: %d Hz\n", rateHz);
            return false;
    }

    // Update ADXL375 data rate
    accel.setDataRate(adxlRate);

    // Calculate new timer interval in microseconds
    uint32_t intervalUs = 1000000 / rateHz;

    // Update hardware timer
    timerAlarmDisable(sampleTimer);
    timerAlarmWrite(sampleTimer, intervalUs, true);
    timerAlarmEnable(sampleTimer);

    currentSampleRateHz = rateHz;
    Serial.printf("Sample rate: %d Hz\n", rateHz);
    return true;
}

// Timing variables
uint32_t lastDisplayTime = 0;
uint32_t lastBLENotifyTime = 0;

// State
bool sensorOk = false;

// Forward declaration (ESP32 doesn't auto-call serialEvent like classic Arduino)
void serialEvent();

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

    // Set up hardware timer for sampling
    // Timer 0, prescaler 80 (80MHz/80 = 1MHz tick rate), count up
    sampleTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(sampleTimer, &onSampleTimer, true);
    // Set initial rate (can be changed at runtime via serial command s1-s4)
    uint32_t initialIntervalUs = 1000000 / ADXL_DEFAULT_SAMPLE_RATE_HZ;
    timerAlarmWrite(sampleTimer, initialIntervalUs, true);
    timerAlarmEnable(sampleTimer);
    Serial.printf("[Setup] Sample timer configured for %d Hz (s1-s4 to change)\n", ADXL_DEFAULT_SAMPLE_RATE_HZ);

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

    // Handle serial commands (ESP32 doesn't auto-call serialEvent)
    serialEvent();

    // Small delay to prevent tight loop (saves power)
    delay(1);
}

/**
 * @brief Handle serial commands
 *
 * Commands:
 *   'r' - Reset peak value
 *   'c' - Calibrate (reset filters)
 *   's1' - Set sample rate to 100 Hz
 *   's2' - Set sample rate to 200 Hz
 *   's3' - Set sample rate to 400 Hz
 *   's4' - Set sample rate to 800 Hz
 *   '?' - Print current status
 */
void serialEvent() {
    static bool expectingRateDigit = false;

    while (Serial.available()) {
        char cmd = Serial.read();

        // Handle rate digit after 's' command
        if (expectingRateDigit) {
            expectingRateDigit = false;
            switch (cmd) {
                case '1': setSampleRate(ADXL_RATE_100HZ); break;
                case '2': setSampleRate(ADXL_RATE_200HZ); break;
                case '3': setSampleRate(ADXL_RATE_400HZ); break;
                case '4': setSampleRate(ADXL_RATE_800HZ); break;
                default:
                    Serial.println("Invalid rate. Use s1=100Hz, s2=200Hz, s3=400Hz, s4=800Hz");
                    break;
            }
            continue;
        }

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

            case 's':
            case 'S':
                expectingRateDigit = true;
                break;

            case '?':
                Serial.printf("Rate: %d Hz | Commands: r=reset peak, c=calibrate, s1-s4=rate, ?=status\n",
                              currentSampleRateHz);
                break;

            default:
                break;
        }
    }
}
