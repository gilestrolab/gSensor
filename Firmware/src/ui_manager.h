/**
 * @file ui_manager.h
 * @brief UI state machine and touch handling
 *
 * Manages screen transitions and dispatches touch events.
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include "settings.h"
#include "touch.h"

/**
 * @brief Touch region definition for hit testing
 */
struct TouchRegion {
    int16_t x1, y1;  ///< Top-left corner
    int16_t x2, y2;  ///< Bottom-right corner
    uint8_t action;  ///< Action ID when tapped
};

// Action IDs for settings screen
constexpr uint8_t ACTION_NONE = 0;
constexpr uint8_t ACTION_TOGGLE_BLE = 1;
constexpr uint8_t ACTION_TOGGLE_SERIAL = 2;
constexpr uint8_t ACTION_BACK = 3;

/**
 * @brief UI Manager for screen state and touch handling
 */
class UIManager {
public:
    UIManager();

    /**
     * @brief Handle a touch event
     *
     * Dispatches touch events to appropriate handlers based on
     * current screen and touch location.
     *
     * Args:
     *     event (TouchEvent&): The touch event to process.
     *     settings (Settings&): Reference to settings for modification.
     *
     * Returns:
     *     bool: True if the event triggered an action.
     */
    bool handleTouch(const TouchEvent& event, Settings& settings);

    /**
     * @brief Get the current screen
     *
     * Returns:
     *     UIScreen: The currently active screen.
     */
    UIScreen getScreen() const;

    /**
     * @brief Set the current screen
     *
     * Args:
     *     screen (UIScreen): The screen to switch to.
     */
    void setScreen(UIScreen screen);

    /**
     * @brief Check if screen just changed (for redraw)
     *
     * Returns:
     *     bool: True if screen changed since last check.
     */
    bool screenChanged();

    /**
     * @brief Signal that a peak reset was requested
     *
     * Returns:
     *     bool: True if reset was requested (clears flag).
     */
    bool peakResetRequested();

private:
    UIScreen currentScreen_;
    bool screenChanged_;
    bool peakResetPending_;

    void handleMainGaugeTouch(const TouchEvent& event, Settings& settings);
    void handleSettingsTouch(const TouchEvent& event, Settings& settings);
    uint8_t hitTest(int16_t x, int16_t y) const;
};

#endif // UI_MANAGER_H
