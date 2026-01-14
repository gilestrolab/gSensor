/**
 * @file signal_processing.h
 * @brief Signal processing utilities for accelerometer data
 *
 * Provides moving average filter and magnitude calculation.
 */

#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include <Arduino.h>
#include <cmath>
#include "config.h"

/**
 * @brief Moving average filter using ring buffer
 *
 * Efficient O(1) implementation that maintains a running sum.
 * Template parameter N sets the window size at compile time.
 *
 * @tparam T Data type (float recommended for accelerometer data)
 * @tparam N Window size (number of samples to average)
 */
template <typename T, size_t N>
class MovingAverage {
public:
    /**
     * @brief Construct a new Moving Average filter
     */
    MovingAverage() : buffer_{}, index_(0), count_(0), sum_(0) {}

    /**
     * @brief Add a new sample to the filter
     *
     * @param value New sample value
     * @return T Current filtered (averaged) value
     */
    T addSample(T value) {
        // Subtract the oldest value from the sum
        sum_ -= buffer_[index_];

        // Store new value and add to sum
        buffer_[index_] = value;
        sum_ += value;

        // Advance index (wrap around)
        index_ = (index_ + 1) % N;

        // Track how many samples we have (up to N)
        if (count_ < N) {
            count_++;
        }

        return getAverage();
    }

    /**
     * @brief Get current averaged value
     *
     * @return T Averaged value (or 0 if no samples)
     */
    T getAverage() const {
        if (count_ == 0) return T(0);
        return sum_ / static_cast<T>(count_);
    }

    /**
     * @brief Reset filter to initial state
     */
    void reset() {
        for (size_t i = 0; i < N; i++) {
            buffer_[i] = T(0);
        }
        index_ = 0;
        count_ = 0;
        sum_ = T(0);
    }

    /**
     * @brief Check if filter has enough samples
     *
     * @return true if buffer is full
     */
    bool isFull() const {
        return count_ >= N;
    }

    /**
     * @brief Get current sample count
     *
     * @return size_t Number of samples in buffer
     */
    size_t getSampleCount() const {
        return count_;
    }

private:
    T buffer_[N];      // Ring buffer
    size_t index_;     // Current write position
    size_t count_;     // Number of samples (up to N)
    T sum_;            // Running sum for efficient averaging
};

/**
 * @brief 3-axis acceleration data structure
 */
struct AccelData {
    float x;  // X-axis acceleration (g)
    float y;  // Y-axis acceleration (g)
    float z;  // Z-axis acceleration (g)

    /**
     * @brief Calculate magnitude of acceleration vector
     *
     * @return float Magnitude in g: sqrt(x^2 + y^2 + z^2)
     */
    float magnitude() const {
        return sqrtf(x * x + y * y + z * z);
    }
};

/**
 * @brief Signal processor for accelerometer data
 *
 * Maintains moving average filters for each axis and magnitude.
 * Also tracks peak values.
 */
class SignalProcessor {
public:
    /**
     * @brief Construct a new Signal Processor
     */
    SignalProcessor();

    /**
     * @brief Process new accelerometer reading
     *
     * @param raw Raw acceleration data from sensor
     * @return AccelData Filtered acceleration data
     */
    AccelData process(const AccelData& raw);

    /**
     * @brief Get current filtered magnitude
     *
     * @return float Filtered magnitude in g
     */
    float getFilteredMagnitude() const;

    /**
     * @brief Get peak magnitude recorded
     *
     * @return float Peak magnitude in g
     */
    float getPeakMagnitude() const;

    /**
     * @brief Reset peak magnitude tracker
     */
    void resetPeak();

    /**
     * @brief Reset all filters and peak tracking
     */
    void reset();

    /**
     * @brief Get the last processed (filtered) data
     *
     * @return const AccelData& Last filtered reading
     */
    const AccelData& getLastFiltered() const;

private:
    MovingAverage<float, MOVING_AVG_WINDOW_SIZE> filterX_;
    MovingAverage<float, MOVING_AVG_WINDOW_SIZE> filterY_;
    MovingAverage<float, MOVING_AVG_WINDOW_SIZE> filterZ_;
    MovingAverage<float, MOVING_AVG_WINDOW_SIZE> filterMag_;

    AccelData lastFiltered_;
    float peakMagnitude_;
};

#endif // SIGNAL_PROCESSING_H
