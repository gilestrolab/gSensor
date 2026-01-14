/**
 * @file display.cpp
 * @brief Implementation of display interface using LovyanGFX
 */

#include "display.h"

Display::Display()
    : tft_()
    , gaugeSprite_(nullptr)
    , lastMagnitude_(0.0f)
    , lastPeak_(0.0f) {
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
    // Draw outer ring decoration
    tft_.drawCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, 118, TFT_DARKGREY);
    tft_.drawCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, 119, TFT_DARKGREY);

    // Draw scale markers around the gauge
    for (int i = 0; i <= 4; i++) {
        float angle = GAUGE_START_ANGLE + (i * 67.5f);
        float rad = angle * DEG_TO_RAD;
        int16_t x1 = DISPLAY_CENTER_X + cos(rad) * (GAUGE_RADIUS_OUTER + 5);
        int16_t y1 = DISPLAY_CENTER_Y + sin(rad) * (GAUGE_RADIUS_OUTER + 5);
        int16_t x2 = DISPLAY_CENTER_X + cos(rad) * (GAUGE_RADIUS_OUTER - 3);
        int16_t y2 = DISPLAY_CENTER_Y + sin(rad) * (GAUGE_RADIUS_OUTER - 3);
        tft_.drawLine(x1, y1, x2, y2, TFT_DARKGREY);
    }

    // Draw unit label
    tft_.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft_.setTextDatum(middle_center);
    tft_.setTextSize(1);
    tft_.drawString("g", DISPLAY_CENTER_X, DISPLAY_CENTER_Y + 35);
}

void Display::update(const AccelData& data, float magnitude, float peak) {
    // Use sprite for flicker-free updates
    gaugeSprite_->fillSprite(TFT_BLACK);

    // Get color based on magnitude
    uint32_t gColor = getGColor(magnitude);

    // Draw gauge arc
    drawGauge(magnitude, gColor);

    // Draw magnitude value
    drawMagnitude(magnitude, gColor);

    // Draw peak indicator
    drawPeak(peak);

    // Draw XYZ values
    drawXYZ(data);

    // Draw outer ring
    gaugeSprite_->drawCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, 118, TFT_DARKGREY);

    // Push sprite to display
    gaugeSprite_->pushSprite(0, 0);

    lastMagnitude_ = magnitude;
    lastPeak_ = peak;
}

void Display::showError(const char* message) {
    clear(TFT_BLACK);

    tft_.setTextColor(TFT_RED, TFT_BLACK);
    tft_.setTextDatum(middle_center);
    tft_.setTextSize(2);

    // Draw error icon (X)
    tft_.drawLine(DISPLAY_CENTER_X - 20, DISPLAY_CENTER_Y - 50,
                  DISPLAY_CENTER_X + 20, DISPLAY_CENTER_Y - 30, TFT_RED);
    tft_.drawLine(DISPLAY_CENTER_X + 20, DISPLAY_CENTER_Y - 50,
                  DISPLAY_CENTER_X - 20, DISPLAY_CENTER_Y - 30, TFT_RED);

    // Draw error message
    tft_.setTextSize(1);
    tft_.drawString(message, DISPLAY_CENTER_X, DISPLAY_CENTER_Y + 10);
}

void Display::showSplash() {
    // Fill with solid blue to verify display works
    tft_.fillScreen(TFT_BLUE);

    // Draw a white circle in center
    tft_.fillCircle(DISPLAY_CENTER_X, DISPLAY_CENTER_Y, 80, TFT_WHITE);

    // Draw text
    tft_.setTextColor(TFT_BLACK, TFT_WHITE);
    tft_.setTextDatum(middle_center);
    tft_.setTextSize(3);
    tft_.drawString("gSENSOR", DISPLAY_CENTER_X, DISPLAY_CENTER_Y);
}

void Display::setBacklight(uint8_t brightness) {
    analogWrite(PIN_TFT_BL, brightness);
}

void Display::backlightOn(bool on) {
    digitalWrite(PIN_TFT_BL, on ? HIGH : LOW);
}

uint32_t Display::getGColor(float g) const {
    if (g < G_THRESHOLD_LOW) {
        return TFT_GREEN;
    } else if (g < G_THRESHOLD_MED) {
        return TFT_YELLOW;
    } else if (g < G_THRESHOLD_HIGH) {
        return TFT_ORANGE;
    } else {
        return TFT_RED;
    }
}

void Display::drawGauge(float value, uint32_t color) {
    // Clamp value to valid range
    if (value < 0) value = 0;
    if (value > GAUGE_MAX_VALUE) value = GAUGE_MAX_VALUE;

    // Calculate arc end angle based on value
    float arcSweep = (value / GAUGE_MAX_VALUE) * (GAUGE_END_ANGLE - GAUGE_START_ANGLE);
    float endAngle = GAUGE_START_ANGLE + arcSweep;

    // Draw background arc (dark)
    fillArc(DISPLAY_CENTER_X, DISPLAY_CENTER_Y,
            GAUGE_START_ANGLE, GAUGE_END_ANGLE,
            GAUGE_RADIUS_OUTER, GAUGE_RADIUS_INNER,
            TFT_DARKGREY);

    // Draw value arc (colored)
    if (arcSweep > 0.5f) {
        fillArc(DISPLAY_CENTER_X, DISPLAY_CENTER_Y,
                GAUGE_START_ANGLE, endAngle,
                GAUGE_RADIUS_OUTER, GAUGE_RADIUS_INNER,
                color);
    }
}

void Display::drawMagnitude(float magnitude, uint32_t color) {
    gaugeSprite_->setTextColor(color, TFT_BLACK);
    gaugeSprite_->setTextDatum(middle_center);

    // Draw large magnitude value
    char buf[16];
    if (magnitude < 10.0f) {
        snprintf(buf, sizeof(buf), "%.2f", magnitude);
    } else if (magnitude < 100.0f) {
        snprintf(buf, sizeof(buf), "%.1f", magnitude);
    } else {
        snprintf(buf, sizeof(buf), "%.0f", magnitude);
    }

    gaugeSprite_->setTextSize(4);
    gaugeSprite_->drawString(buf, DISPLAY_CENTER_X, DISPLAY_CENTER_Y - 10);

    // Draw unit
    gaugeSprite_->setTextColor(TFT_DARKGREY, TFT_BLACK);
    gaugeSprite_->setTextSize(2);
    gaugeSprite_->drawString("g", DISPLAY_CENTER_X, DISPLAY_CENTER_Y + 25);
}

void Display::drawPeak(float peak) {
    gaugeSprite_->setTextColor(TFT_ORANGE, TFT_BLACK);
    gaugeSprite_->setTextDatum(middle_center);
    gaugeSprite_->setTextSize(1);

    char buf[24];
    snprintf(buf, sizeof(buf), "PEAK: %.1fg", peak);
    gaugeSprite_->drawString(buf, DISPLAY_CENTER_X, 30);
}

void Display::drawXYZ(const AccelData& data) {
    gaugeSprite_->setTextColor(TFT_WHITE, TFT_BLACK);
    gaugeSprite_->setTextDatum(middle_center);
    gaugeSprite_->setTextSize(1);

    char buf[32];
    snprintf(buf, sizeof(buf), "X:%.1f Y:%.1f Z:%.1f",
             data.x, data.y, data.z);
    gaugeSprite_->drawString(buf, DISPLAY_CENTER_X, DISPLAY_HEIGHT - 25);
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
