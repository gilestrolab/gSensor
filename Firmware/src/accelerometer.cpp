/**
 * @file accelerometer.cpp
 * @brief Implementation of ADXL375 accelerometer driver
 */

#include "accelerometer.h"

Accelerometer::Accelerometer()
    : i2cBus_(0)  // Use I2C bus 0 (ESP32-C3 has one hardware I2C)
    , adxl_(nullptr)
    , initialized_(false) {
}

bool Accelerometer::begin() {
    // Initialize I2C on custom pins (JST connector)
    // Reason: ESP32-C3 Wire library allows specifying custom SDA/SCL pins
    if (!i2cBus_.begin(PIN_ADXL_SDA, PIN_ADXL_SCL, 400000)) {
        if (DEBUG_ENABLED) {
            Serial.println("ERROR: Failed to initialize I2C bus");
        }
        return false;
    }

    // Create ADXL375 instance with custom I2C bus
    adxl_ = new Adafruit_ADXL375(12345, &i2cBus_);

    // Initialize sensor
    if (!adxl_->begin(ADXL375_I2C_ADDR)) {
        if (DEBUG_ENABLED) {
            Serial.print("ERROR: ADXL375 not found at address 0x");
            Serial.println(ADXL375_I2C_ADDR, HEX);
        }
        delete adxl_;
        adxl_ = nullptr;
        return false;
    }

    // Verify device ID
    uint8_t deviceId = getDeviceID();
    if (deviceId != 0xE5) {
        if (DEBUG_ENABLED) {
            Serial.print("WARNING: Unexpected device ID: 0x");
            Serial.println(deviceId, HEX);
        }
    }

    // Configure sensor for high-speed operation
    // Reason: We want responsive readings for impact detection
    adxl_->setDataRate(ADXL3XX_DATARATE_100_HZ);

    initialized_ = true;

    if (DEBUG_ENABLED) {
        Serial.println("ADXL375 initialized successfully");
        Serial.print("  SDA: GPIO");
        Serial.println(PIN_ADXL_SDA);
        Serial.print("  SCL: GPIO");
        Serial.println(PIN_ADXL_SCL);
        Serial.print("  Address: 0x");
        Serial.println(ADXL375_I2C_ADDR, HEX);
        Serial.print("  Device ID: 0x");
        Serial.println(deviceId, HEX);
    }

    return true;
}

bool Accelerometer::read(AccelData& data) {
    if (!initialized_ || adxl_ == nullptr) {
        data = {0.0f, 0.0f, 0.0f};
        return false;
    }

    sensors_event_t event;
    if (!adxl_->getEvent(&event)) {
        return false;
    }

    // Convert from m/s^2 to g
    // Reason: The Adafruit library returns values in m/s^2, but for
    // impact/vibration analysis, g-force is more intuitive
    constexpr float MS2_TO_G = 1.0f / 9.80665f;

    // Apply calibration offsets to remove sensor bias
    data.x = (event.acceleration.x * MS2_TO_G) - OFFSET_X;
    data.y = (event.acceleration.y * MS2_TO_G) - OFFSET_Y;
    data.z = (event.acceleration.z * MS2_TO_G) - OFFSET_Z;

    return true;
}

bool Accelerometer::isConnected() const {
    return initialized_ && adxl_ != nullptr;
}

uint8_t Accelerometer::getDeviceID() const {
    if (adxl_ == nullptr) {
        return 0;
    }
    return adxl_->getDeviceID();
}

void Accelerometer::setDataRate(adxl3xx_dataRate_t rate) {
    if (adxl_ != nullptr) {
        adxl_->setDataRate(rate);
    }
}

void Accelerometer::getRawValues(int16_t& x, int16_t& y, int16_t& z) {
    if (adxl_ == nullptr) {
        x = y = z = 0;
        return;
    }

    // Read raw values directly from registers
    // Reason: Useful for debugging and calibration
    sensors_event_t event;
    adxl_->getEvent(&event);

    // Convert back from m/s^2 to raw counts for debugging
    // ADXL375 sensitivity: 49 mg/LSB
    constexpr float G_TO_LSB = 1.0f / ADXL375_SCALE_FACTOR;
    constexpr float MS2_TO_G = 1.0f / 9.80665f;

    x = static_cast<int16_t>(event.acceleration.x * MS2_TO_G * G_TO_LSB);
    y = static_cast<int16_t>(event.acceleration.y * MS2_TO_G * G_TO_LSB);
    z = static_cast<int16_t>(event.acceleration.z * MS2_TO_G * G_TO_LSB);
}
