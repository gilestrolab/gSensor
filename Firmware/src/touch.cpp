/**
 * @file touch.cpp
 * @brief CST816D touch controller driver implementation
 */

#include "touch.h"

// CST816D register addresses
constexpr uint8_t CST816_REG_GESTURE = 0x01;
constexpr uint8_t CST816_REG_FINGER_NUM = 0x02;
constexpr uint8_t CST816_REG_X_HIGH = 0x03;
constexpr uint8_t CST816_REG_X_LOW = 0x04;
constexpr uint8_t CST816_REG_Y_HIGH = 0x05;
constexpr uint8_t CST816_REG_Y_LOW = 0x06;

// Volatile flag for interrupt handling
volatile bool touchInterrupt = false;

/**
 * @brief Interrupt handler for touch events
 */
void IRAM_ATTR touchISR() {
    touchInterrupt = true;
}

TouchManager::TouchManager()
    : touchWire_(1)  // Use second I2C bus
    , currentEvent_{TouchGesture::NONE, 0, 0, 0}
    , initialized_(false)
    , touching_(false)
    , lastTouching_(false)
    , touchX_(0)
    , touchY_(0)
    , touchStartTime_(0)
    , eventPending_(false) {
}

bool TouchManager::begin() {
    // Initialize I2C for touch controller
    touchWire_.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, 400000);

    // Reset touch controller with proper timing
    pinMode(PIN_TOUCH_RST, OUTPUT);
    digitalWrite(PIN_TOUCH_RST, LOW);
    delay(10);
    digitalWrite(PIN_TOUCH_RST, HIGH);
    delay(300);  // Longer delay for CST816D to fully initialize

    // Setup interrupt pin
    pinMode(PIN_TOUCH_INT, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_TOUCH_INT), touchISR, FALLING);

    // Verify communication by reading a byte
    touchWire_.beginTransmission(TOUCH_I2C_ADDR);
    uint8_t error = touchWire_.endTransmission();

    if (error != 0) {
        Serial.printf("[Touch] CST816D not found at 0x%02X (error %d)\n", TOUCH_I2C_ADDR, error);
        initialized_ = false;
        return false;
    }

    // Disable auto low-power mode (important for reliable touch detection)
    touchWire_.beginTransmission(TOUCH_I2C_ADDR);
    touchWire_.write(0xFE);  // Register address
    touchWire_.write(0xFF);  // Disable auto sleep
    touchWire_.endTransmission();

    Serial.println("[Touch] CST816D initialized successfully");
    initialized_ = true;
    return true;
}

void TouchManager::update() {
    // Skip if touch controller not initialized
    if (!initialized_) {
        return;
    }

    static uint32_t lastPollTime = 0;
    uint32_t now = millis();

    // Check for touch interrupt (new touch event)
    if (touchInterrupt) {
        Serial.println("[Touch] Interrupt fired!");
        touchInterrupt = false;
        readTouch();
        lastPollTime = now;
    }
    // Poll touch state while actively touching to detect release
    else if (touching_) {
        readTouch();
        lastPollTime = now;
    }
    // Also poll periodically even without interrupt (every 100ms)
    // Some CST816 variants don't fire interrupts reliably
    else if (now - lastPollTime >= 100) {
        lastPollTime = now;
        // Check if there's actually a touch happening
        touchWire_.beginTransmission(TOUCH_I2C_ADDR);
        touchWire_.write(CST816_REG_FINGER_NUM);
        if (touchWire_.endTransmission() == 0) {
            if (touchWire_.requestFrom(TOUCH_I2C_ADDR, (uint8_t)1) == 1) {
                uint8_t fingerNum = touchWire_.read();
                if (fingerNum > 0) {
                    Serial.println("[Touch] Polling detected touch!");
                    readTouch();
                }
            }
        }
    }

    // Process gesture when touch ends
    if (lastTouching_ && !touching_) {
        Serial.println("[Touch] Touch ended, processing gesture...");
        processGesture();
    }

    lastTouching_ = touching_;
}

bool TouchManager::readTouch() {
    // Read touch data from CST816D
    touchWire_.beginTransmission(TOUCH_I2C_ADDR);
    touchWire_.write(CST816_REG_FINGER_NUM);
    uint8_t err = touchWire_.endTransmission();
    if (err != 0) {
        Serial.printf("[Touch] I2C write error: %d\n", err);
        return false;
    }

    uint8_t bytesRead = touchWire_.requestFrom(TOUCH_I2C_ADDR, (uint8_t)5);
    if (bytesRead != 5) {
        Serial.printf("[Touch] I2C read error: got %d bytes, expected 5\n", bytesRead);
        return false;
    }

    uint8_t fingerNum = touchWire_.read();
    uint8_t xHigh = touchWire_.read();
    uint8_t xLow = touchWire_.read();
    uint8_t yHigh = touchWire_.read();
    uint8_t yLow = touchWire_.read();

    // Extract coordinates (mask off event bits in high byte)
    int16_t x = ((xHigh & 0x0F) << 8) | xLow;
    int16_t y = ((yHigh & 0x0F) << 8) | yLow;

    bool wasTouch = (fingerNum > 0);

    if (wasTouch && !touching_) {
        // Touch started
        touching_ = true;
        touchX_ = x;
        touchY_ = y;
        touchStartTime_ = millis();
        Serial.printf("[Touch] START at (%d, %d)\n", x, y);
    } else if (wasTouch) {
        // Touch continues - update position
        touchX_ = x;
        touchY_ = y;
    } else {
        // Touch ended
        if (touching_) {
            Serial.println("[Touch] RELEASE detected");
        }
        touching_ = false;
    }

    return true;
}

void TouchManager::processGesture() {
    uint32_t duration = millis() - touchStartTime_;

    if (duration >= TOUCH_LONG_PRESS_MS) {
        currentEvent_.gesture = TouchGesture::LONG_PRESS;
        Serial.printf("[Touch] LONG_PRESS at (%d, %d), duration=%lu ms\n",
                      touchX_, touchY_, duration);
    } else if (duration >= 50 && duration < TOUCH_TAP_THRESHOLD_MS) {
        // Minimum 50ms to filter out noise
        currentEvent_.gesture = TouchGesture::TAP;
        Serial.printf("[Touch] TAP at (%d, %d), duration=%lu ms\n",
                      touchX_, touchY_, duration);
    } else {
        currentEvent_.gesture = TouchGesture::NONE;
        Serial.printf("[Touch] Ignored touch, duration=%lu ms (too short)\n", duration);
        return;
    }

    currentEvent_.x = touchX_;
    currentEvent_.y = touchY_;
    currentEvent_.timestamp = millis();
    eventPending_ = true;
}

TouchEvent TouchManager::getEvent() {
    if (eventPending_) {
        eventPending_ = false;
        return currentEvent_;
    }
    return {TouchGesture::NONE, 0, 0, 0};
}

bool TouchManager::isTouching() const {
    return touching_;
}
