/**
 * @file ui_manager.cpp
 * @brief UI state machine implementation
 */

#include "ui_manager.h"

// Touch regions for settings screen (x1, y1, x2, y2, action)
// Layout based on 240x240 display
static const TouchRegion settingsRegions[] = {
    {30, 60, 210, 100, ACTION_TOGGLE_BLE},      // BLE toggle row
    {30, 110, 210, 150, ACTION_TOGGLE_SERIAL},  // Serial toggle row
    {70, 195, 170, 230, ACTION_BACK}            // Back button
};
static const size_t NUM_SETTINGS_REGIONS = sizeof(settingsRegions) / sizeof(settingsRegions[0]);

UIManager::UIManager()
    : currentScreen_(UIScreen::MAIN_GAUGE)
    , screenChanged_(true)
    , peakResetPending_(false) {
}

bool UIManager::handleTouch(const TouchEvent& event, Settings& settings) {
    if (event.gesture == TouchGesture::NONE) {
        return false;
    }

    switch (currentScreen_) {
        case UIScreen::MAIN_GAUGE:
            handleMainGaugeTouch(event, settings);
            return true;

        case UIScreen::SETTINGS:
            handleSettingsTouch(event, settings);
            return true;
    }

    return false;
}

void UIManager::handleMainGaugeTouch(const TouchEvent& event, Settings& settings) {
    if (event.gesture == TouchGesture::TAP) {
        // Any tap on main gauge opens settings
        currentScreen_ = UIScreen::SETTINGS;
        screenChanged_ = true;
        Serial.println("[UI] Opening settings");
    } else if (event.gesture == TouchGesture::LONG_PRESS) {
        // Long press resets peak
        peakResetPending_ = true;
        Serial.println("[UI] Peak reset requested");
    }
}

void UIManager::handleSettingsTouch(const TouchEvent& event, Settings& settings) {
    if (event.gesture != TouchGesture::TAP) {
        return;
    }

    uint8_t action = hitTest(event.x, event.y);

    switch (action) {
        case ACTION_TOGGLE_BLE:
            settings.bleEnabled = !settings.bleEnabled;
            Serial.printf("[UI] BLE %s\n", settings.bleEnabled ? "enabled" : "disabled");
            break;

        case ACTION_TOGGLE_SERIAL:
            settings.serialEnabled = !settings.serialEnabled;
            Serial.printf("[UI] Serial %s\n", settings.serialEnabled ? "enabled" : "disabled");
            break;

        case ACTION_BACK:
            currentScreen_ = UIScreen::MAIN_GAUGE;
            screenChanged_ = true;
            Serial.println("[UI] Back to main gauge");
            break;

        default:
            // Tap outside buttons - also go back
            currentScreen_ = UIScreen::MAIN_GAUGE;
            screenChanged_ = true;
            break;
    }
}

uint8_t UIManager::hitTest(int16_t x, int16_t y) const {
    for (size_t i = 0; i < NUM_SETTINGS_REGIONS; i++) {
        const TouchRegion& r = settingsRegions[i];
        if (x >= r.x1 && x <= r.x2 && y >= r.y1 && y <= r.y2) {
            return r.action;
        }
    }
    return ACTION_NONE;
}

UIScreen UIManager::getScreen() const {
    return currentScreen_;
}

void UIManager::setScreen(UIScreen screen) {
    if (currentScreen_ != screen) {
        currentScreen_ = screen;
        screenChanged_ = true;
    }
}

bool UIManager::screenChanged() {
    if (screenChanged_) {
        screenChanged_ = false;
        return true;
    }
    return false;
}

bool UIManager::peakResetRequested() {
    if (peakResetPending_) {
        peakResetPending_ = false;
        return true;
    }
    return false;
}
