/**
 * @file signal_processing.cpp
 * @brief Implementation of signal processing utilities
 */

#include "signal_processing.h"

SignalProcessor::SignalProcessor()
    : filterX_()
    , filterY_()
    , filterZ_()
    , filterMag_()
    , lastFiltered_{0.0f, 0.0f, 0.0f}
    , peakMagnitude_(0.0f) {
}

AccelData SignalProcessor::process(const AccelData& raw) {
    // Apply moving average filter to each axis
    lastFiltered_.x = filterX_.addSample(raw.x);
    lastFiltered_.y = filterY_.addSample(raw.y);
    lastFiltered_.z = filterZ_.addSample(raw.z);

    // Calculate and filter magnitude
    // Reason: We filter magnitude separately rather than computing from filtered
    // axes to preserve the actual magnitude response (filtering axes separately
    // can underestimate magnitude during rapid changes)
    float rawMagnitude = raw.magnitude();
    float filteredMag = filterMag_.addSample(rawMagnitude);

    // Update peak tracking
    if (filteredMag > peakMagnitude_) {
        peakMagnitude_ = filteredMag;
    }

    return lastFiltered_;
}

float SignalProcessor::getFilteredMagnitude() const {
    return filterMag_.getAverage();
}

float SignalProcessor::getPeakMagnitude() const {
    return peakMagnitude_;
}

void SignalProcessor::resetPeak() {
    peakMagnitude_ = 0.0f;
}

void SignalProcessor::reset() {
    filterX_.reset();
    filterY_.reset();
    filterZ_.reset();
    filterMag_.reset();
    lastFiltered_ = {0.0f, 0.0f, 0.0f};
    peakMagnitude_ = 0.0f;
}

const AccelData& SignalProcessor::getLastFiltered() const {
    return lastFiltered_;
}
