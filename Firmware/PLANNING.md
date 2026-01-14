# gSENSOR Firmware - Planning Document

## Project Overview
Firmware for an ESP32-based g-sensor device that records and displays force measurements from a laboratory shaker using an ADXL375 high-g accelerometer.

## Hardware Components

### Main Board: ESP32-2424S012
- **MCU:** ESP32-C3-MINI-1U (4MB Flash)
- **Display:** GC9A01 240x240 round LCD (SPI interface)
- **Touch:** CST816D capacitive touch (unused in this project)
- **Power:** USB-C or 360mAh 3.7V battery
- **Connectivity:** WiFi 802.11 b/g/n, Bluetooth 5.0

### Accelerometer: ADXL375 (Adafruit Breakout)
- **Range:** ±200g (fixed)
- **Sensitivity:** 49 mg/LSB
- **Interface:** I2C (address 0x53)
- **Features:** STEMMA QT/Qwiic connector

## Pin Mapping

### Display (SPI - Fixed on board)
| Function | GPIO |
|----------|------|
| MOSI     | 7    |
| SCLK     | 6    |
| CS       | 10   |
| DC       | 2    |
| Backlight| 3    |

### ADXL375 I2C (via JST connector)
| JST Pin | GPIO | ADXL375 |
|---------|------|---------|
| GND     | GND  | GND     |
| 3.3V    | 3.3V | VIN     |
| TX      | 21   | SDA     |
| RX      | 20   | SCL     |

**Note:** ESP32-C3 only has GPIO0-21. There is no GPIO22!

## Software Architecture

### Framework
- PlatformIO with Arduino framework
- ESP32-C3 target
- LovyanGFX library for display (same as manufacturer's factory firmware)

### Module Structure
```
src/
├── main.cpp              # Application entry point
├── config.h              # Hardware configuration & calibration
├── display.h/.cpp        # GC9A01 display driver (LovyanGFX)
├── accelerometer.h/.cpp  # ADXL375 driver
└── signal_processing.h/.cpp  # Moving average filter
```

### Key Libraries
- LovyanGFX: Display driver (works reliably with ESP32-2424S012)
- Adafruit ADXL375: Accelerometer driver
- Adafruit BusIO: I2C abstraction

## Calibration

The ADXL375 has per-unit zero-g offsets that must be calibrated. Current offsets (measured with sensor flat, screen facing up):

```cpp
OFFSET_X = 0.35f   // X axis offset
OFFSET_Y = -0.60f  // Y axis offset
OFFSET_Z = 0.70f   // Z axis offset (gravity should read 1.0g)
```

To recalibrate:
1. Place sensor flat with screen facing up
2. Read raw X, Y, Z values from serial output
3. X and Y offsets = raw values (should be ~0)
4. Z offset = raw Z value - 1.0 (should be ~1.0g from gravity)

## Design Decisions

### Display Library Choice
- **LovyanGFX** used instead of TFT_eSPI
- Matches manufacturer's factory firmware configuration
- TFT_eSPI had SPI communication issues with this board

### Signal Processing
- Moving average filter with 10-sample window
- Magnitude calculated from raw data before filtering
- Peak tracking with manual reset capability

### Display Update Strategy
- Sample accelerometer at 100 Hz
- Update display at 20 Hz (50ms interval)
- Use sprite buffering to prevent flicker
- Color-coded magnitude (green < 10g, yellow < 50g, orange < 100g, red > 100g)

### I2C Configuration
- Repurpose UART pins (TX/RX) on JST connector as I2C
- GPIO21 = SDA, GPIO20 = SCL
- 400kHz I2C clock

## Serial Interface

**Baud rate:** 115200

**Important:** ESP32-C3 requires USB CDC to be enabled for serial output over USB:
```ini
build_flags =
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
```

**Output format (CSV):**
```
timestamp,x,y,z,magnitude,peak
7257,-0.002,-0.375,-0.896,0.972,1.013
```

**Commands:**
- `r` or `R` - Reset peak value
- `c` or `C` - Reset filters (recalibrate)

**Serial Plotter:**
A Python plotting tool is available in `tools/serial_plotter.py`:
```bash
cd tools
pip install -r requirements.txt
python serial_plotter.py --port /dev/ttyACM0
```

## Future Enhancements
- [ ] Data logging to SPIFFS
- [ ] WiFi data streaming
- [ ] Auto-calibration routine
- [ ] Configurable sample rates
- [ ] FFT analysis for vibration frequency

## References
- [ESP32-2424S012 Info](https://homeding.github.io/boards/esp32c3/jczn-esp32-2424s012.htm)
- [ADXL375 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL375.PDF)
- [LovyanGFX Library](https://github.com/lovyan03/LovyanGFX)
- [Manufacturer Package](http://pan.jczn1688.com/directlink/1/ESP32%20module/1.28inch_ESP32-2424S012.zip)
