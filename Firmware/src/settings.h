/**
 * @file settings.h
 * @brief Settings structure and UI screen definitions
 *
 * Contains runtime settings for communication modes and UI state.
 * Note: Settings are not persisted - they reset to defaults on boot.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include "config.h"

/**
 * @brief UI screen enumeration
 */
enum class UIScreen {
    MAIN_GAUGE,
    SETTINGS
};

/**
 * @brief Runtime settings structure
 *
 * These settings control communication modes and are reset
 * to defaults on each power cycle.
 */
struct Settings {
    bool bleEnabled = true;      ///< Enable BLE advertising and notifications
    bool serialEnabled = true;   ///< Enable serial debug output
    uint32_t sampleRateHz = ADXL_DEFAULT_SAMPLE_RATE_HZ;  ///< Current acquisition sample rate

    /**
     * @brief Reset settings to defaults
     */
    void reset() {
        bleEnabled = true;
        serialEnabled = true;
        sampleRateHz = ADXL_DEFAULT_SAMPLE_RATE_HZ;
    }
};

#endif // SETTINGS_H
