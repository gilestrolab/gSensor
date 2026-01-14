/**
 * @file display.cpp
 * @brief Implementation of display interface using LovyanGFX
 *
 * Racing HUD style with smooth anti-aliased fonts.
 */

#include "display.h"

// Smooth font includes from LovyanGFX
#include <lgfx/v1/LGFX_Sprite.hpp>

// Default gauge max value
constexpr float DEFAULT_GAUGE_MAX = 10.0f;

Display::Display()
    : tft_()
    , gaugeSprite_(nullptr)
    , lastMagnitude_(0.0f)
    , lastPeak_(0.0f)
    , gaugeMax_(DEFAULT_GAUGE_MAX) {
}

bool Display::begin() {
    // Initialize display
    tft_.init();

    // Fill with test color
    tft_.fillScreen(TFT_BLUE);

    // Turn on backlight AFTER display init (as per factory sample)
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);

    delay(100);

    // Clear to black
    tft_.fillScreen(TFT_BLACK);

    // Create sprite for smooth gauge updates
    gaugeSprite_ = new LGFX_Sprite(&tft_);
    gaugeSprite_->createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    gaugeSprite_->setColorDepth(16);

    if (DEBUG_ENABLED) {
        Serial.println("Display initialized (LovyanGFX)");
        Serial.print("  Resolution: ");
        Serial.print(DISPLAY_WIDTH);
        Serial.print("x");
        Serial.println(DISPLAY_HEIGHT);
    }

    return true;
}

void Display::clear(uint32_t color) {
    tft_.fillScreen(color);
}

void Display::drawStaticUI() {
    // Modern minimal style - no decorative elements
    // The gauge and text are drawn in update() via sprite
    tft_.fillScreen(UI_BG_PRIMARY);
}

void Display::update(const AccelData& data, float magnitude, float peak) {
    // Use sprite for flicker-free updates
    gaugeSprite_->fillSprite(UI_BG_PRIMARY);

    // Reset text size to 1 (smooth fonts handle their own sizing)
    gaugeSprite_->setTextSize(1);

    // Adapt gauge max if magnitude exceeds current range
    // Use nice round numbers: 10, 20, 50, 100, 200
    if (magnitude > gaugeMax_) {
        if (magnitude <= 10.0f) {
            gaugeMax_ = 10.0f;
        } else if (magnitude <= 20.0f) {
            gaugeMax_ = 20.0f;
        } else if (magnitude <= 50.0f) {
            gaugeMax_ = 50.0f;
        } else if (magnitude <= 100.0f) {
            gaugeMax_ = 100.0f;
        } else {
            gaugeMax_ = 200.0f;
        }
    }

    // Get color based on magnitude
    uint32_t gColor = getGColor(magnitude);

    // Draw Racing HUD style gauge
    drawGaugeHUD(magnitude, gColor);

    // Draw inner accent ring
    drawAccentRing(gColor);

    // Draw magnitude value
    drawMagnitudeSmooth(magnitude, gColor);

    // Draw peak indicator with accent bar
    drawPeakHUD(peak);

    // Draw XYZ values with bottom accent bar
    drawXYZHUD(data);

    // Push sprite to display
    gaugeSprite_->pushSprite(0, 0);

    lastMagnitude_ = magnitude;
    lastPeak_ = peak;
}

void Display::showError(const char* message) {
    clear(TFT_BLACK);

    tft_.setTextColor(TFT_RED, TFT_BLACK);
    tft_.setTextDatum(middle_center);

    // Draw error icon (X) with thicker lines
    for (int i = -1; i <= 1; i++) {
        tft_.drawLine(DISPLAY_CENTER_X - 20 + i, DISPLAY_CENTER_Y - 50,
                      DISPLAY_CENTER_X + 20 + i, DISPLAY_CENTER_Y - 30, TFT_RED);
        tft_.drawLine(DISPLAY_CENTER_X + 20 + i, DISPLAY_CENTER_Y - 50,
                      DISPLAY_CENTER_X - 20 + i, DISPLAY_CENTER_Y - 30, TFT_RED);
    }

    // Draw error message
    tft_.setTextSize(1);
    tft_.drawString(message, DISPLAY_CENTER_X, DISPLAY_CENTER_Y + 10);
}

void Display::showSplash() {
    // Racing HUD style splash screen
    tft_.fillScreen(UI_BG_PRIMARY);

    // Draw segmented accent arc (matching main gauge style)
    constexpr int NUM_SEGMENTS = 20;
    constexpr float SEGMENT_GAP = 3.0f;
    constexpr float TOTAL_ARC = 270.0f;
    constexpr float SEGMENT_ANGLE = (TOTAL_ARC - (NUM_SEGMENTS * SEGMENT_GAP)) / NUM_SEGMENTS;

    float angle = 135.0f;
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        float rad1 = angle * DEG_TO_RAD;
        float rad2 = (angle + SEGMENT_ANGLE) * DEG_TO_RAD;

        // Draw segment
        for (float a = angle; a < angle + SEGMENT_ANGLE; a += 2.0f) {
            float rad = a * DEG_TO_RAD;
            int16_t x1 = DISPLAY_CENTER_X + cos(rad) * 90;
            int16_t y1 = DISPLAY_CENTER_Y + sin(rad) * 90;
            int16_t x2 = DISPLAY_CENTER_X + cos(rad) * 105;
            int16_t y2 = DISPLAY_CENTER_Y + sin(rad) * 105;
            tft_.drawLine(x1, y1, x2, y2, UI_ACCENT);
        }
        angle += SEGMENT_ANGLE + SEGMENT_GAP;
    }

    // Draw text with smooth font
    tft_.setTextColor(UI_TEXT_PRIMARY, UI_BG_PRIMARY);
    tft_.setTextDatum(middle_center);
    tft_.setFont(&fonts::FreeSansBold18pt7b);
    tft_.drawString("gSENSOR", DISPLAY_CENTER_X, DISPLAY_CENTER_Y);

    // Version/subtitle
    tft_.setTextColor(UI_TEXT_MUTED, UI_BG_PRIMARY);
    tft_.setFont(&fonts::FreeSans9pt7b);
    tft_.drawString("High-G Accelerometer", DISPLAY_CENTER_X, DISPLAY_CENTER_Y + 35);
}

void Display::setBacklight(uint8_t brightness) {
    analogWrite(PIN_TFT_BL, brightness);
}

void Display::backlightOn(bool on) {
    digitalWrite(PIN_TFT_BL, on ? HIGH : LOW);
}

void Display::resetGaugeMax() {
    gaugeMax_ = DEFAULT_GAUGE_MAX;
}

uint32_t Display::getGColor(float g) const {
    if (g < G_THRESHOLD_LOW) {
        return COLOR_LOW_G;
    } else if (g < G_THRESHOLD_MED) {
        return COLOR_MED_G;
    } else if (g < G_THRESHOLD_HIGH) {
        return COLOR_HIGH_G;
    } else {
        return COLOR_EXTREME_G;
    }
}

void Display::drawGaugeHUD(float value, uint32_t color) {
    // Racing HUD style: segmented arc with gaps
    if (value < 0) value = 0;
    if (value > gaugeMax_) value = gaugeMax_;

    constexpr int NUM_SEGMENTS = 20;
    constexpr float SEGMENT_GAP = 3.0f;  // Degrees gap between segments
    constexpr float TOTAL_ARC = GAUGE_END_ANGLE - GAUGE_START_ANGLE;  // 270 degrees
    constexpr float SEGMENT_ANGLE = (TOTAL_ARC - (NUM_SEGMENTS * SEGMENT_GAP)) / NUM_SEGMENTS;

    int filledSegments = (int)((value / gaugeMax_) * NUM_SEGMENTS);

    float angle = GAUGE_START_ANGLE;
    for (int i = 0; i < NUM_SEGMENTS; i++) {
        // Segment is filled if below current value
        uint32_t segColor = (i < filledSegments) ? color : UI_GAUGE_BG;

        fillArc(DISPLAY_CENTER_X, DISPLAY_CENTER_Y,
                angle, angle + SEGMENT_ANGLE,
                105, 85,  // Outer and inner radius
                segColor);

        angle += SEGMENT_ANGLE + SEGMENT_GAP;
    }
}

void Display::drawAccentRing(uint32_t color) {
    // Dim inner accent ring
    uint16_t dimColor = tft_.color565(
        ((color >> 11) & 0x1F) * 8 / 24,  // R component dimmed
        ((color >> 5) & 0x3F) * 4 / 24,   // G component dimmed
        (color & 0x1F) * 8 / 24           // B component dimmed
    );
    gaugeSprite_->drawCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, 60, dimColor);
}

void Display::drawMagnitudeSmooth(float magnitude, uint32_t color) {
    gaugeSprite_->setTextColor(color, UI_BG_PRIMARY);
    gaugeSprite_->setTextDatum(middle_center);

    char buf[16];
    if (magnitude < 10.0f) {
        snprintf(buf, sizeof(buf), "%.2f", magnitude);
    } else if (magnitude < 100.0f) {
        snprintf(buf, sizeof(buf), "%.1f", magnitude);
    } else {
        snprintf(buf, sizeof(buf), "%.0f", magnitude);
    }

    // Large value using smooth font
    gaugeSprite_->setFont(&fonts::FreeSansBold24pt7b);
    gaugeSprite_->drawString(buf, DISPLAY_CENTER_X, DISPLAY_CENTER_Y - 5);

    // Unit label
    gaugeSprite_->setTextColor(UI_TEXT_SECONDARY, UI_BG_PRIMARY);
    gaugeSprite_->setFont(&fonts::FreeSans12pt7b);
    gaugeSprite_->drawString("G", DISPLAY_CENTER_X, DISPLAY_CENTER_Y + 35);
}

void Display::drawPeakHUD(float peak) {
    // Draw framed box for peak
    int16_t boxW = 110;
    int16_t boxH = 28;
    int16_t boxX = DISPLAY_CENTER_X - boxW / 2;
    int16_t boxY = 8;

    // Box background and border
    gaugeSprite_->fillRoundRect(boxX, boxY, boxW, boxH, 4, UI_BG_SECONDARY);
    gaugeSprite_->drawRoundRect(boxX, boxY, boxW, boxH, 4, UI_ACCENT);

    // Peak text with smooth font
    gaugeSprite_->setTextColor(UI_ACCENT, UI_BG_SECONDARY);
    gaugeSprite_->setTextDatum(middle_center);
    gaugeSprite_->setFont(&fonts::FreeSans9pt7b);

    char buf[24];
    snprintf(buf, sizeof(buf), "PEAK %.1f", peak);
    gaugeSprite_->drawString(buf, DISPLAY_CENTER_X, boxY + boxH / 2 + 2);
}

void Display::drawXYZHUD(const AccelData& data) {
    // Accent bar at bottom
    gaugeSprite_->fillRect(35, DISPLAY_HEIGHT - 22, 170, 2, UI_TEXT_MUTED);

    // XYZ values with smooth font
    gaugeSprite_->setTextColor(UI_TEXT_SECONDARY, UI_BG_PRIMARY);
    gaugeSprite_->setTextDatum(middle_center);
    gaugeSprite_->setFont(&fonts::FreeSans9pt7b);

    char buf[40];
    snprintf(buf, sizeof(buf), "X%+.0f Y%+.0f Z%+.0f",
             data.x, data.y, data.z);
    gaugeSprite_->drawString(buf, DISPLAY_CENTER_X, DISPLAY_HEIGHT - 38);
}

// Legacy functions kept for compatibility but unused
void Display::drawGauge(float value, uint32_t color) {
    drawGaugeHUD(value, color);
}

void Display::drawMagnitude(float magnitude, uint32_t color) {
    drawMagnitudeSmooth(magnitude, color);
}

void Display::drawPeak(float peak) {
    drawPeakHUD(peak);
}

void Display::drawXYZ(const AccelData& data) {
    drawXYZHUD(data);
}

void Display::fillArc(int16_t x, int16_t y, float start_angle, float end_angle,
                      int16_t r_outer, int16_t r_inner, uint32_t color) {
    float step = 2.0f;

    for (float angle = start_angle; angle < end_angle; angle += step) {
        float nextAngle = angle + step;
        if (nextAngle > end_angle) nextAngle = end_angle;

        float rad1 = angle * DEG_TO_RAD;
        float rad2 = nextAngle * DEG_TO_RAD;

        int16_t x1 = x + cos(rad1) * r_outer;
        int16_t y1 = y + sin(rad1) * r_outer;
        int16_t x2 = x + cos(rad2) * r_outer;
        int16_t y2 = y + sin(rad2) * r_outer;

        int16_t x3 = x + cos(rad1) * r_inner;
        int16_t y3 = y + sin(rad1) * r_inner;
        int16_t x4 = x + cos(rad2) * r_inner;
        int16_t y4 = y + sin(rad2) * r_inner;

        gaugeSprite_->fillTriangle(x1, y1, x2, y2, x3, y3, color);
        gaugeSprite_->fillTriangle(x2, y2, x3, y3, x4, y4, color);
    }
}

void Display::prepareScreen() {
    tft_.fillScreen(UI_BG_PRIMARY);
}

void Display::drawSettingsScreen(const Settings& settings, bool bleConnected) {
    // Use sprite for flicker-free updates
    gaugeSprite_->fillSprite(UI_BG_PRIMARY);

    // Reset to default text size (important when mixing with smooth fonts)
    gaugeSprite_->setTextSize(1);

    // Header with smooth font
    gaugeSprite_->setTextColor(UI_TEXT_PRIMARY, UI_BG_PRIMARY);
    gaugeSprite_->setTextDatum(middle_center);
    gaugeSprite_->setFont(&fonts::FreeSansBold12pt7b);
    gaugeSprite_->drawString("SETTINGS", DISPLAY_CENTER_X, 30);

    // BLE toggle button
    drawToggleButton(30, 60, 180, 40, "BLE", settings.bleEnabled);

    // Serial toggle button
    drawToggleButton(30, 110, 180, 40, "Serial", settings.serialEnabled);

    // Status line with smooth font
    gaugeSprite_->setTextColor(UI_TEXT_SECONDARY, UI_BG_PRIMARY);
    gaugeSprite_->setTextDatum(middle_center);
    gaugeSprite_->setFont(&fonts::FreeSans9pt7b);

    if (settings.bleEnabled) {
        if (bleConnected) {
            gaugeSprite_->setTextColor(COLOR_LOW_G, UI_BG_PRIMARY);
            gaugeSprite_->drawString("BLE: Connected", DISPLAY_CENTER_X, 165);
        } else {
            gaugeSprite_->drawString("BLE: Advertising...", DISPLAY_CENTER_X, 165);
        }
    } else {
        gaugeSprite_->setTextColor(UI_TEXT_MUTED, UI_BG_PRIMARY);
        gaugeSprite_->drawString("BLE: Disabled", DISPLAY_CENTER_X, 165);
    }

    // Back button
    drawBackButton(70, 195, 100, 35);

    // Push sprite to display
    gaugeSprite_->pushSprite(0, 0);
}

void Display::drawToggleButton(int16_t x, int16_t y, int16_t w, int16_t h,
                                const char* label, bool active) {
    // Button background
    uint16_t bgColor = active ? UI_BG_SECONDARY : UI_BG_PRIMARY;
    uint16_t borderColor = active ? UI_ACCENT : UI_TEXT_MUTED;

    gaugeSprite_->fillRoundRect(x, y, w, h, 8, bgColor);
    gaugeSprite_->drawRoundRect(x, y, w, h, 8, borderColor);

    // Toggle indicator (left side)
    int16_t indicatorX = x + 15;
    int16_t indicatorY = y + h / 2;
    if (active) {
        gaugeSprite_->fillCircle(indicatorX, indicatorY, 8, UI_ACCENT);
    } else {
        gaugeSprite_->drawCircle(indicatorX, indicatorY, 8, UI_TEXT_MUTED);
    }

    // Label text with smooth font
    gaugeSprite_->setTextColor(active ? UI_TEXT_PRIMARY : UI_TEXT_SECONDARY, bgColor);
    gaugeSprite_->setTextDatum(middle_left);
    gaugeSprite_->setFont(&fonts::FreeSans12pt7b);
    gaugeSprite_->drawString(label, x + 35, y + h / 2);

    // Status text on right
    gaugeSprite_->setTextDatum(middle_right);
    gaugeSprite_->setFont(&fonts::FreeSans9pt7b);
    gaugeSprite_->drawString(active ? "ON" : "OFF", x + w - 15, y + h / 2);
}

void Display::drawBackButton(int16_t x, int16_t y, int16_t w, int16_t h) {
    // Button with accent border
    gaugeSprite_->fillRoundRect(x, y, w, h, 6, UI_BG_SECONDARY);
    gaugeSprite_->drawRoundRect(x, y, w, h, 6, UI_ACCENT);

    // Back text with smooth font
    gaugeSprite_->setTextColor(UI_ACCENT, UI_BG_SECONDARY);
    gaugeSprite_->setTextDatum(middle_center);
    gaugeSprite_->setFont(&fonts::FreeSans12pt7b);
    gaugeSprite_->drawString("BACK", x + w / 2, y + h / 2);
}
