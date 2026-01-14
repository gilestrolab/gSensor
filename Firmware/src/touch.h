/**
 * @file touch.h
 * @brief Touch controller interface for CST816D
 *
 * Provides touch input handling for the ESP32-2424S012 board.
 */

#ifndef TOUCH_H
#define TOUCH_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

/**
 * @brief Touch gesture types
 */
enum class TouchGesture {
    NONE,
    TAP,
    LONG_PRESS
};

/**
 * @brief Touch event data
 */
struct TouchEvent {
    TouchGesture gesture;
    int16_t x;
    int16_t y;
    uint32_t timestamp;
};

/**
 * @brief Touch controller manager for CST816D
 *
 * Handles I2C communication with the touch controller and
 * provides gesture detection (tap, long press).
 */
class TouchManager {
public:
    TouchManager();

    /**
     * @brief Initialize the touch controller
     *
     * Sets up I2C bus and configures interrupt pin.
     *
     * Returns:
     *     bool: True if initialization successful.
     */
    bool begin();

    /**
     * @brief Check if touch controller was initialized
     */
    bool isInitialized() const { return initialized_; }

    /**
     * @brief Update touch state (call in main loop)
     *
     * Reads touch data when available and updates internal state.
     */
    void update();

    /**
     * @brief Get the latest touch event
     *
     * Returns:
     *     TouchEvent: The touch event (gesture type and coordinates).
     */
    TouchEvent getEvent();

    /**
     * @brief Check if touch is currently active
     *
     * Returns:
     *     bool: True if finger is on screen.
     */
    bool isTouching() const;

private:
    TwoWire touchWire_;
    TouchEvent currentEvent_;

    bool initialized_;
    bool touching_;
    bool lastTouching_;
    int16_t touchX_;
    int16_t touchY_;
    uint32_t touchStartTime_;
    bool eventPending_;

    bool readTouch();
    void processGesture();
};

#endif // TOUCH_H
