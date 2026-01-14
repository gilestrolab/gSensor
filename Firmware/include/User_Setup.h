/**
 * TFT_eSPI User Setup for ESP32-2424S012 Board
 *
 * This configuration file sets up the GC9A01 240x240 round display
 * connected via SPI on the ESP32-C3.
 */

#ifndef USER_SETUP_H
#define USER_SETUP_H

// ==================== Display Driver ====================
#define GC9A01_DRIVER

// ==================== Display Resolution ====================
#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// ==================== ESP32-C3 SPI Pin Configuration ====================
// These pins are fixed on the ESP32-2424S012 board
#define TFT_MOSI 7   // SPI MOSI (data out to display)
#define TFT_SCLK 6   // SPI Clock
#define TFT_CS   10  // Chip select
#define TFT_DC   2   // Data/Command
#define TFT_RST  -1  // Reset (not connected, use software reset)
#define TFT_BL   3   // Backlight control

// ==================== SPI Configuration ====================
#define SPI_FREQUENCY  40000000  // 40 MHz SPI clock
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000

// ==================== Display Orientation ====================
// Rotation 0 = 0 degrees, 1 = 90, 2 = 180, 3 = 270
// The display is typically mounted upside down, so use 180 degrees
#define TFT_INVERSION_ON
// Note: Rotation is set in code via tft.setRotation()

// ==================== Color Configuration ====================
#define TFT_RGB_ORDER TFT_BGR  // GC9A01 uses BGR color order

// ==================== Font Configuration ====================
#define LOAD_GLCD   // Standard 8x8 fixed width font
#define LOAD_FONT2  // 16 pixel high font
#define LOAD_FONT4  // 26 pixel high font
#define LOAD_FONT6  // 48 pixel high numeric font
#define LOAD_FONT7  // 7 segment 48 pixel font
#define LOAD_FONT8  // 75 pixel high numeric font
#define LOAD_GFXFF  // FreeFonts

#define SMOOTH_FONT // Enable anti-aliased fonts

#endif // USER_SETUP_H
