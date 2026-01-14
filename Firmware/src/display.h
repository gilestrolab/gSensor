/**
 * @file display.h
 * @brief Display interface for round GC9A01 LCD using LovyanGFX
 *
 * Provides high-level drawing functions for g-sensor UI.
 */

#ifndef DISPLAY_H
#define DISPLAY_H

// LGFX_USE_V1 defined via build_flags in platformio.ini
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "config.h"
#include "signal_processing.h"

/**
 * @brief LovyanGFX display class for ESP32-2424S012
 *
 * Configured exactly as per manufacturer's factory sample.
 */
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX(void) {
        // SPI bus configuration
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 20000000;
            cfg.spi_3wire = true;
            cfg.use_lock = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk = 6;
            cfg.pin_mosi = 7;
            cfg.pin_miso = -1;
            cfg.pin_dc = 2;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel configuration
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 10;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
            cfg.memory_width = 240;
            cfg.memory_height = 240;
            cfg.panel_width = 240;
            cfg.panel_height = 240;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits = 1;
            cfg.readable = false;
            cfg.invert = true;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};

/**
 * @brief Display manager for g-sensor UI
 */
class Display {
public:
    Display();
    bool begin();
    void clear(uint32_t color = TFT_BLACK);
    void drawStaticUI();
    void update(const AccelData& data, float magnitude, float peak);
    void showError(const char* message);
    void showSplash();
    void setBacklight(uint8_t brightness);
    void backlightOn(bool on = true);

private:
    LGFX tft_;
    LGFX_Sprite* gaugeSprite_;
    float lastMagnitude_;
    float lastPeak_;

    uint32_t getGColor(float g) const;
    void drawGauge(float value, uint32_t color);
    void drawMagnitude(float magnitude, uint32_t color);
    void drawPeak(float peak);
    void drawXYZ(const AccelData& data);
    void fillArc(int16_t x, int16_t y, float start_angle, float end_angle,
                 int16_t r_outer, int16_t r_inner, uint32_t color);
};

#endif // DISPLAY_H
