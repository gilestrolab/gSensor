/**
 * @file accelerometer.h
 * @brief ADXL375 accelerometer driver
 *
 * Wrapper around Adafruit ADXL375 library with custom I2C bus support.
 */

#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADXL375.h>
#include "config.h"
#include "signal_processing.h"

/**
 * @brief ADXL375 accelerometer interface
 *
 * Manages initialization and reading of the ADXL375 high-g accelerometer
 * connected via software I2C on the JST connector pins.
 */
class Accelerometer {
public:
    /**
     * @brief Construct a new Accelerometer interface
     */
    Accelerometer();

    /**
     * @brief Initialize the accelerometer
     *
     * Sets up I2C communication on custom pins and configures the sensor.
     *
     * @return true if initialization successful
     * @return false if sensor not found or communication error
     */
    bool begin();

    /**
     * @brief Read acceleration data from sensor
     *
     * @param data Output structure to store acceleration values
     * @return true if read successful
     * @return false if communication error
     */
    bool read(AccelData& data);

    /**
     * @brief Check if accelerometer is connected and responding
     *
     * @return true if sensor responds to I2C queries
     */
    bool isConnected() const;

    /**
     * @brief Get sensor device ID
     *
     * @return uint8_t Device ID (should be 0xE5 for ADXL375)
     */
    uint8_t getDeviceID() const;

    /**
     * @brief Set data rate
     *
     * @param rate Data rate enum from Adafruit library
     */
    void setDataRate(adxl3xx_dataRate_t rate);

    /**
     * @brief Get raw sensor values for debugging
     *
     * @param x Raw X value
     * @param y Raw Y value
     * @param z Raw Z value
     */
    void getRawValues(int16_t& x, int16_t& y, int16_t& z);

private:
    TwoWire i2cBus_;           // Custom I2C bus instance
    Adafruit_ADXL375* adxl_;   // Adafruit driver instance
    bool initialized_;
};

#endif // ACCELEROMETER_H
